#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<32> b1;
    bit<32> b2;
    bit<32> b3;
    bit<32> b4;
}

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("data") 
    data_t data;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".start") state start {
        packet.extract(hdr.data);
        transition accept;
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".setb1") action setb1(bit<32> val) {
        hdr.data.b1 = val;
    }
    @name(".noop") action noop() {
    }
    @name(".setb3") action setb3(bit<32> val) {
        hdr.data.b3 = val;
    }
    @name(".on_hit") action on_hit() {
    }
    @name(".on_miss") action on_miss() {
    }
    @name(".setb2") action setb2(bit<32> val) {
        hdr.data.b2 = val;
    }
    @name(".setb4") action setb4(bit<32> val) {
        hdr.data.b4 = val;
    }
    @name(".A1") table A1 {
        actions = {
            setb1;
            noop;
        }
        key = {
            hdr.data.f1: ternary;
        }
    }
    @name(".A2") table A2 {
        actions = {
            setb3;
            noop;
        }
        key = {
            hdr.data.b1: ternary;
        }
    }
    @name(".A3") table A3 {
        actions = {
            on_hit;
            on_miss;
        }
        key = {
            hdr.data.f2: ternary;
        }
    }
    @name(".A4") table A4 {
        actions = {
            on_hit;
            on_miss;
        }
        key = {
            hdr.data.f2: ternary;
        }
    }
    @name(".B1") table B1 {
        actions = {
            setb2;
            noop;
        }
        key = {
            hdr.data.f2: ternary;
        }
    }
    @name(".B2") table B2 {
        actions = {
            setb4;
            noop;
        }
        key = {
            hdr.data.b2: ternary;
        }
    }
    apply {
        if (hdr.data.b1 == 32w0) {
            A1.apply();
            A2.apply();
            if (hdr.data.f1 == 32w0) {
                switch (A3.apply().action_run) {
                    on_hit: {
                        A4.apply();
                    }
                }

            }
        }
        B1.apply();
        B2.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit(hdr.data);
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

V1Switch(__ParserImpl(), __verifyChecksumImpl(), ingress(), __egressImpl(), __computeChecksumImpl(), __DeparserImpl()) main;
