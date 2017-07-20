#include <core.p4>
#include <v1model.p4>

struct m_t {
    bit<32> f1;
    bit<32> f2;
}

struct __metadataImpl {
    @name("m") 
    m_t                 m;
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".start") state start {
        transition accept;
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".a1") action a1() {
        meta.m.f1 = 32w1;
    }
    @name(".a2") action a2() {
        meta.m.f2 = 32w2;
    }
    @name(".t1") table t1 {
        actions = {
            a1();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    @name(".t2") table t2 {
        actions = {
            a2();
            @defaultonly NoAction();
        }
        key = {
            meta.m.f1: exact @name("meta.m.f1") ;
        }
        default_action = NoAction();
    }
    apply {
        t1.apply();
        t2.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
    }
}

control __verifyChecksumImpl(in __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

control __computeChecksumImpl(inout __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), __egressImpl(), __computeChecksumImpl(), __DeparserImpl()) main;
