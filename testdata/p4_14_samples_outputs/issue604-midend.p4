#include <core.p4>
#include <v1model.p4>

struct metadata {
}

struct headers {
}

extern extern_test {
    void my_extern_method();
    extern_test();
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("NoAction") action NoAction_0() {
    }
    @name("my_extern_inst") extern_test() my_extern_inst;
    @name(".a") action a_0() {
        my_extern_inst.my_extern_method();
    }
    @name(".t") table t {
        actions = {
            a_0();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    apply {
        t.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
    }
}

control verifyChecksum(in headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
