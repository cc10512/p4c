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

#ifndef _BACKENDS_BMV2_CONVERTERRORCODE_H_
#define _BACKENDS_BMV2_CONVERTERRORCODE_H_

#include "ir/ir.h"
#include "lib/json.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/referenceMap.h"

namespace BMV2 {


class ErrorCodesVisitor : public Inspector {
    Util::JsonArray* errors;
    using ErrorValue = unsigned int;
    using ErrorCodesMap = std::unordered_map<const IR::IDeclaration *, ErrorValue>;
    ErrorCodesMap*    errorCodesMap;
 protected:
    Util::JsonArray* pushNewArray(Util::JsonArray* parent);
 public:
    // we map error codes to numerical values for bmv2
    bool preorder(const IR::Type_Error* errors) override;
    void postorder(const IR::P4Program* program) override;
    explicit ErrorCodesVisitor(Util::JsonArray* errors, ErrorCodesMap* errorCodesMap) :
        errors(errors), errorCodesMap(errorCodesMap)
    { CHECK_NULL(errors); CHECK_NULL(errorCodesMap); }
};

} // namespace BMV2

#endif