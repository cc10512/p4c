/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "parser.h"

namespace BMV2 {

Util::IJson* Parser::toJson(const IR::P4Parser* parser) {
    auto result = new Util::JsonObject();
    result->emplace("name", "parser");  // at least in simple_router this name is hardwired
    result->emplace("id", nextId("parser"));
    result->emplace("init_state", IR::ParserState::start);
    auto states = mkArrayField(result, "parse_states");

    for (auto state : parser->states) {
        auto json = toJson(state);
        if (json != nullptr)
            states->append(json);
    }
    return result;
}

Util::IJson* Parser::convertParserStatement(const IR::StatOrDecl* stat) {
    auto result = new Util::JsonObject();
    auto params = mkArrayField(result, "parameters");
    if (stat->is<IR::AssignmentStatement>()) {
        auto assign = stat->to<IR::AssignmentStatement>();
        result->emplace("op", "set");
        auto l = conv->convertLeftValue(assign->left);
        auto type = typeMap->getType(assign->left, true);
        bool convertBool = type->is<IR::Type_Boolean>();
        auto r = conv->convert(assign->right, true, true, convertBool);
        params->append(l);
        params->append(r);
        return result;
    } else if (stat->is<IR::MethodCallStatement>()) {
        auto mce = stat->to<IR::MethodCallStatement>()->methodCall;
        auto minst = P4::MethodInstance::resolve(mce, refMap, typeMap);
        if (minst->is<P4::ExternMethod>()) {
            auto extmeth = minst->to<P4::ExternMethod>();
            if (extmeth->method->name.name == corelib.packetIn.extract.name) {
                result->emplace("op", "extract");
                if (mce->arguments->size() == 1) {
                    auto arg = mce->arguments->at(0);
                    auto argtype = typeMap->getType(arg, true);
                    if (!argtype->is<IR::Type_Header>()) {
                        ::error("%1%: extract only accepts arguments with header types, not %2%",
                                arg, argtype);
                        return result;
                    }
                    auto param = new Util::JsonObject();
                    params->append(param);
                    cstring type;
                    Util::IJson* j = nullptr;

                    if (arg->is<IR::Member>()) {
                        auto mem = arg->to<IR::Member>();
                        auto baseType = typeMap->getType(mem->expr, true);
                        if (baseType->is<IR::Type_Stack>()) {
                            if (mem->member == IR::Type_Stack::next) {
                                type = "stack";
                                j = conv->convert(mem->expr);
                            } else {
                                BUG("%1%: unsupported", mem);
                            }
                        }
                    }
                    if (j == nullptr) {
                        type = "regular";
                        j = conv->convert(arg);
                    }
                    auto value = j->to<Util::JsonObject>()->get("value");
                    param->emplace("type", type);
                    param->emplace("value", value);
                    return result;
                }
            }
        } else if (minst->is<P4::ExternFunction>()) {
            auto extfn = minst->to<P4::ExternFunction>();
            if (extfn->method->name.name == IR::ParserState::verify) {
                result->emplace("op", "verify");
                BUG_CHECK(mce->arguments->size() == 2, "%1%: Expected 2 arguments", mce);
                {
                    auto cond = mce->arguments->at(0);
                    // false means don't wrap in an outer expression object, which is not needed
                    // here
                    auto jexpr = conv->convert(cond, true, false);
                    params->append(jexpr);
                }
                {
                    auto error = mce->arguments->at(1);
                    // false means don't wrap in an outer expression object, which is not needed
                    // here
                    auto jexpr = conv->convert(error, true, false);
                    params->append(jexpr);
                }
                return result;
            }
        } else if (minst->is<P4::BuiltInMethod>()) {
            auto bi = minst->to<P4::BuiltInMethod>();
            if (bi->name == IR::Type_Header::setValid || bi->name == IR::Type_Header::setInvalid) {
                auto mem = new IR::Member(bi->appliedTo, "$valid$");
                typeMap->setType(mem, IR::Type_Void::get());
                auto jexpr = conv->convert(mem, true, false);
                result->emplace("op", "set");
                params->append(jexpr);

                auto bl = new IR::BoolLiteral(bi->name == IR::Type_Header::setValid);
                auto r = conv->convert(bl, true, true, true);
                params->append(r);
                return result;
            }
        }
    }
    ::error("%1%: not supported in parser on this target", stat);
    return result;
}

// Operates on a select keyset
void Parser::convertSimpleKey(const IR::Expression* keySet,
                                     mpz_class& value, mpz_class& mask) const {
    if (keySet->is<IR::Mask>()) {
        auto mk = keySet->to<IR::Mask>();
        if (!mk->left->is<IR::Constant>()) {
            ::error("%1% must evaluate to a compile-time constant", mk->left);
            return;
        }
        if (!mk->right->is<IR::Constant>()) {
            ::error("%1% must evaluate to a compile-time constant", mk->right);
            return;
        }
        value = mk->left->to<IR::Constant>()->value;
        mask = mk->right->to<IR::Constant>()->value;
    } else if (keySet->is<IR::Constant>()) {
        value = keySet->to<IR::Constant>()->value;
        mask = -1;
    } else if (keySet->is<IR::BoolLiteral>()) {
        value = keySet->to<IR::BoolLiteral>()->value ? 1 : 0;
        mask = -1;
    } else {
        ::error("%1% must evaluate to a compile-time constant", keySet);
        value = 0;
        mask = 0;
    }
}

unsigned Parser::combine(const IR::Expression* keySet,
                                const IR::ListExpression* select,
                                mpz_class& value, mpz_class& mask) const {
    // From the BMv2 spec: For values and masks, make sure that you
    // use the correct format. They need to be the concatenation (in
    // the right order) of all byte padded fields (padded with 0
    // bits). For example, if the transition key consists of a 12-bit
    // field and a 2-bit field, each value will need to have 3 bytes
    // (2 for the first field, 1 for the second one). If the
    // transition value is 0xaba, 0x3, the value attribute will be set
    // to 0x0aba03.
    // Return width in bytes
    value = 0;
    mask = 0;
    unsigned totalWidth = 0;
    if (keySet->is<IR::DefaultExpression>()) {
        return totalWidth;
    } else if (keySet->is<IR::ListExpression>()) {
        auto le = keySet->to<IR::ListExpression>();
        BUG_CHECK(le->components.size() == select->components.size(),
                  "%1%: mismatched select", select);
        unsigned index = 0;

        bool noMask = true;
        for (auto it = select->components.begin();
             it != select->components.end(); ++it) {
            auto e = *it;
            auto keyElement = le->components.at(index);

            auto type = typeMap->getType(e, true);
            int width = type->width_bits();
            BUG_CHECK(width > 0, "%1%: unknown width", e);

            mpz_class key_value, mask_value;
            convertSimpleKey(keyElement, key_value, mask_value);
            unsigned w = 8 * ROUNDUP(width, 8);
            totalWidth += ROUNDUP(width, 8);
            value = Util::shift_left(value, w) + key_value;
            if (mask_value != -1) {
                mask = Util::shift_left(mask, w) + mask_value;
                noMask = false;
            }
            LOG3("Shifting " << " into key " << key_value << " &&& " << mask_value <<
                 " result is " << value << " &&& " << mask);
            index++;
        }

        if (noMask)
            mask = -1;
        return totalWidth;
    } else {
        BUG_CHECK(select->components.size() == 1, "%1%: mismatched select/label", select);
        convertSimpleKey(keySet, value, mask);
        auto type = typeMap->getType(select->components.at(0), true);
        return type->width_bits() / 8;
    }
}

Util::IJson* Parser::stateName(IR::ID state) {
    if (state.name == IR::ParserState::accept) {
        return Util::JsonValue::null;
    } else if (state.name == IR::ParserState::reject) {
        ::warning("Explicit transition to %1% not supported on this target", state);
        return Util::JsonValue::null;
    } else {
        return new Util::JsonValue(state.name);
    }
}

Util::IJson* Parser::toJson(const IR::ParserState* state) {
    if (state->name == IR::ParserState::reject || state->name == IR::ParserState::accept)
        return nullptr;

    auto result = new Util::JsonObject();
    result->emplace("name", extVisibleName(state));
    result->emplace("id", nextId("parse_states"));
    auto operations = mkArrayField(result, "parser_ops");
    for (auto s : state->components) {
        auto j = convertParserStatement(s);
        operations->append(j);
    }

    Util::IJson* key;
    auto transitions = mkArrayField(result, "transitions");
    if (state->selectExpression != nullptr) {
        if (state->selectExpression->is<IR::SelectExpression>()) {
            auto se = state->selectExpression->to<IR::SelectExpression>();
            key = conv->convert(se->select, false);
            for (auto sc : se->selectCases) {
                auto trans = new Util::JsonObject();
                mpz_class value, mask;
                unsigned bytes = combine(sc->keyset, se->select, value, mask);
                if (mask == 0) {
                    trans->emplace("value", "default");
                    trans->emplace("mask", Util::JsonValue::null);
                    trans->emplace("next_state", stateName(sc->state->path->name));
                } else {
                    trans->emplace("value", stringRepr(value, bytes));
                    if (mask == -1)
                        trans->emplace("mask", Util::JsonValue::null);
                    else
                        trans->emplace("mask", stringRepr(mask, bytes));
                    trans->emplace("next_state", stateName(sc->state->path->name));
                }
                transitions->append(trans);
            }
        } else if (state->selectExpression->is<IR::PathExpression>()) {
            auto pe = state->selectExpression->to<IR::PathExpression>();
            key = new Util::JsonArray();
            auto trans = new Util::JsonObject();
            trans->emplace("value", "default");
            trans->emplace("mask", Util::JsonValue::null);
            trans->emplace("next_state", stateName(pe->path->name));
            transitions->append(trans);
        } else {
            BUG("%1%: unexpected selectExpression", state->selectExpression);
        }
    } else {
        key = new Util::JsonArray();
        auto trans = new Util::JsonObject();
        trans->emplace("value", "default");
        trans->emplace("mask", Util::JsonValue::null);
        trans->emplace("next_state", Util::JsonValue::null);
        transitions->append(trans);
    }
    result->emplace("transition_key", key);
    return result;
}

bool Parser::preorder(const IR::PackageBlock* block) {
    for (auto it : block->constantValue) {
        if (it.second->is<IR::ParserBlock>()) {
            visit(it.second->getNode());
        }
    }
    return false;
}

bool Parser::preorder(const IR::P4Parser* parser) {
    auto parserJson = toJson(parser);
    parsers->append(parserJson);
    return false;
}

} // namespace BMV2