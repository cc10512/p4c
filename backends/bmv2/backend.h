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

#ifndef _BACKENDS_BMV2_BACKEND_H_
#define _BACKENDS_BMV2_BACKEND_H_

#include "ir/ir.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/gc.h"
#include "lib/json.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "analyzer.h"
#include "helpers.h"
#include "expression.h"
#include "frontends/p4/coreLibrary.h"
#include "midend/convertEnums.h"
#include "frontends/common/model.h"
#include "frontends/p4/fromv1.0/v1model.h"

namespace BMV2 {

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
    void setDestination(const IR::IDeclaration* meter, const IR::Expression* destination);
    void setTable(const IR::IDeclaration* meter, const IR::P4Table* table);
    void setSize(const IR::IDeclaration* meter, unsigned size);
};

class Backend {
    ExpressionConverter*             conv;
    P4::ConvertEnums::EnumMapping*   enumMap;
    P4::P4CoreLibrary&               corelib;
    P4::ReferenceMap                 refMap;
    P4::TypeMap                      typeMap;
    ProgramParts                     structure;
    Util::JsonObject                 toplevel;
    P4::V2Model&                     model;
    P4V1::V1Model&                   v1model;
    DirectMeterMap                   meterMap;
    using ErrorValue = unsigned int;
    using ErrorCodesMap = std::unordered_map<const IR::IDeclaration *, ErrorValue>;
    ErrorCodesMap                    errorCodesMap;
    unsigned                         scalars_width = 0;
    const unsigned                   boolWidth = 1;
    cstring                          scalarsName;

 public:
    Util::JsonArray*                 calculations;
    Util::JsonArray*                 checksums;
    Util::JsonArray*                 counters;
    Util::JsonArray*                 field_lists;
    Util::JsonArray*                 learn_lists;
    Util::JsonArray*                 meter_arrays;
    Util::JsonArray*                 register_arrays;
    Util::JsonArray*                 headerTypes;
    Util::JsonArray*                 headerInstances;
    Util::JsonArray*                 headerStacks;
    Util::JsonArray*                 parsers;
    Util::JsonArray*                 pipelines;
    Util::JsonArray*                 deparsers;
    Util::JsonArray*                 actions;
    Util::JsonArray*                 externs;
    Util::JsonArray*                 errors;
    Util::JsonArray*                 enums;

    Util::JsonObject*                scalarsStruct;
    Util::JsonArray*                 scalarFields;

    // We place scalar user metadata fields (i.e., bit<>, bool)
    // in the "scalars" metadata object, so we may need to rename
    // these fields.  This map holds the new names.
    std::map<const IR::StructField*, cstring> scalarMetadataFields;

 protected:
    void addMetaInformation();
    void addEnums(Util::JsonArray* enums);
    void addLocals();
    void createScalars();
    void padScalars();
    void genExternMethod(Util::JsonArray* result, P4::ExternMethod *em);
    Backend::ErrorValue retrieveErrorValue(const IR::Member* mem) const;
    void convertActionBody(const IR::Vector<IR::StatOrDecl>* body, Util::JsonArray* result);
    void createActions(Util::JsonArray* actions);

 public:
    explicit Backend(P4::ConvertEnums::EnumMapping* enumMap) :
        enumMap(enumMap), corelib(P4::P4CoreLibrary::instance),
        model(P4::V2Model::instance), v1model(P4V1::V1Model::instance)
    {}
    void run(const IR::ToplevelBlock* block);
    void serialize(std::ostream& out) const
    { toplevel.serialize(out); }
    ExpressionConverter* getExpressionConverter() { return conv; };
    DirectMeterMap&      getMeterMap() { return meterMap; }
    P4::ReferenceMap&    getRefMap()   { return refMap; }
    P4::TypeMap&         getTypeMap()  { return typeMap; }
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_BACKEND_H_ */