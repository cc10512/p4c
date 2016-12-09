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

#ifndef _BACKENDS_BMV2_JSONCONVERTER_H_
#define _BACKENDS_BMV2_JSONCONVERTER_H_

#include "lib/json.h"
#include "frontends/common/options.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/common/16model.h"
#include "analyzer.h"
#include <iomanip>
// Currently we are requiring a v1model to be used

// This is based on the specification of the BMv2 JSON input format
// https://github.com/p4lang/behavioral-model/blob/master/docs/JSON_format.md

namespace BMV2 {

class BMV2_Model : public  ::P4_16::V2Model {
 private:
    struct TableAttributes_Model {
        TableAttributes_Model() : 
                tableImplementation("implementation"),
                directCounter("counters"),
                directMeter("meters"), size("size"),
                supportTimeout("support_timeout") {}
        ::Model::Elem tableImplementation;
        ::Model::Elem directCounter;
        ::Model::Elem directMeter;
        ::Model::Elem size;
        ::Model::Elem supportTimeout;
        const unsigned defaultTableSize = 1024;
    };
    
    struct TableImplementation_Model {
        TableImplementation_Model() :
                actionProfile("action_profile"),
                actionSelector("action_selector") {}
        ::Model::Elem actionProfile;
        ::Model::Elem actionSelector;
    };

 public:
    BMV2_Model(::P4_16::V2Model *v2model) :
            tableAttributes(), tableImplementations(),
            selectorMatchType("selector"), rangeMatchType("range") {
        this->parsers = v2model->parsers;
        this->controls = v2model->controls;
        this->externs = v2model->externs;
    }
    
    ::Model::Elem             selectorMatchType;
    ::Model::Elem             rangeMatchType;
    TableAttributes_Model     tableAttributes;
    TableImplementation_Model tableImplementations;
};

class ExpressionConverter;

class DirectMeterMap final {
 public:
    struct DirectMeterInfo {
        const IR::Expression* destinationField;
        const IR::P4Table* table;
        unsigned tableSize;

        DirectMeterInfo() : destinationField(nullptr), table(nullptr), tableSize(0) {}
    };

 private:
    // key is declaration of direct meter
    std::map<const IR::IDeclaration*, DirectMeterInfo*> directMeter;
    DirectMeterInfo* createInfo(const IR::IDeclaration* meter);
 public:
    DirectMeterInfo* getInfo(const IR::IDeclaration* meter);
    void setDestination(const IR::IDeclaration* meter,
                        const IR::Expression* destination);
    void setTable(const IR::IDeclaration* meter, const IR::P4Table* table);
    void setSize(const IR::IDeclaration* meter, unsigned size);
};

class JsonConverter final {
 public:
    const CompilerOptions& options;
    Util::JsonObject       toplevel;  // output is constructed here

    // TODO(pierce): going away
    P4V1::V1Model&         v1model;

    BMV2_Model             model;

    P4::P4CoreLibrary&     corelib;
    P4::ReferenceMap*      refMap;
    P4::TypeMap*           typeMap;
    ProgramParts           structure;
    cstring                dropAction = ".drop";
    cstring                scalarsName;  // name of struct in JSON holding all scalars
    unsigned               dropActionId;
    IR::ToplevelBlock*     toplevelBlock;
    ExpressionConverter*   conv;
    DirectMeterMap         meterMap;
    const IR::Parameter*   headerParameter;
    const IR::Parameter*   userMetadataParameter;
    const IR::Parameter*   stdMetadataParameter;
    cstring                jsonMetadataParameterName = "standard_metadata";

 private:
    Util::JsonArray *headerTypes;
    std::map<cstring, cstring> headerTypesCreated;
    Util::JsonArray *headerInstances;
    Util::JsonArray *headerStacks;
    friend class ExpressionConverter;

 protected:
    void pushFields(cstring prefix, const IR::Type_StructLike *st,
                    Util::JsonArray *fields);
    cstring createJsonType(const IR::Type_StructLike *type);
    unsigned nextId(cstring group);
    void addLocals();
    void addTypesAndInstances(const IR::Parameter *param, const IR::Type_Struct *type);
    void convertActionBody(const IR::Vector<IR::StatOrDecl>* body,
                           Util::JsonArray* result, Util::JsonArray* fieldLists,
                           Util::JsonArray* calculations, Util::JsonArray* learn_lists);
    Util::IJson* convertTable(const CFG::TableNode* node, Util::JsonArray* counters, Util::JsonArray* meters);
    Util::IJson* convertIf(const CFG::IfNode* node, cstring parent);
    Util::JsonArray* createActions(Util::JsonArray* fieldLists,
                                   Util::JsonArray* calculations,
                                   Util::JsonArray* learn_lists);
    Util::IJson* toJson(const IR::P4Parser* cont);
    Util::IJson* toJson(const IR::ParserState* state);
    void convertDeparserBody(const IR::Vector<IR::StatOrDecl>* body,
                             Util::JsonArray* result);
    Util::IJson* convertDeparser(const IR::P4Control* state);
    Util::IJson* convertParserStatement(const IR::StatOrDecl* stat);
    Util::IJson* convertControl(const IR::ControlBlock* block, cstring name,
                                Util::JsonArray* counters, Util::JsonArray* meters,
                                Util::JsonArray* registers, Util::JsonArray *externs);
    cstring createCalculation(cstring algo, const IR::Expression* fields,
                              Util::JsonArray* calculations);
    Util::IJson* nodeName(const CFG::Node* node) const;
    cstring convertHashAlgorithm(cstring algorithm) const;
    // Return 'true' if the table is 'simple'
    bool handleTableImplementation(const IR::Property* implementation,
                                   const IR::Key* key,
                                   Util::JsonObject* table);
    void addToFieldList(const IR::Expression* expr, Util::JsonArray* fl);
    // returns id of created field list
    int createFieldList(const IR::Expression* expr, cstring group,
                        cstring listName, Util::JsonArray* fieldLists);
    void generateUpdate(const IR::BlockStatement *block,
                        Util::JsonArray* checksums, Util::JsonArray* calculations);
    void generateUpdate(const IR::P4Control* cont,
                        Util::JsonArray* checksums, Util::JsonArray* calculations);

    // Operates on a select keyset
    void convertSimpleKey(const IR::Expression* keySet,
                          mpz_class& value, mpz_class& mask) const;
    unsigned combine(const IR::Expression* keySet,
                     const IR::ListExpression* select,
                     mpz_class& value, mpz_class& mask) const;
    void buildCfg(IR::P4Control* cont);

 public:
    explicit JsonConverter(const CompilerOptions& options, ::P4_16::V2Model *v2model);
    void convert(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                 IR::ToplevelBlock *toplevel);
    void serialize(std::ostream& out) const
    { toplevel.serialize(out); }
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_JSONCONVERTER_H_ */
