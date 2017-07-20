#include <core.p4>
#include <v1model.p4>

struct intrinsic_metadata_t {
    bit<4>  mcast_grp;
    bit<4>  egress_rid;
    bit<16> mcast_hash;
    bit<32> lf_field_list;
}

header cpu_header_t {
    bit<8> device;
    bit<8> reason;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct __metadataImpl {
    @name("intrinsic_metadata") 
    intrinsic_metadata_t intrinsic_metadata;
    @name("standard_metadata") 
    standard_metadata_t  standard_metadata;
}

struct __headersImpl {
    @name("cpu_header") 
    cpu_header_t cpu_header;
    @name("ethernet") 
    ethernet_t   ethernet;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    bit<64> tmp_0;
    @name(".parse_cpu_header") state parse_cpu_header {
        packet.extract<cpu_header_t>(hdr.cpu_header);
        transition parse_ethernet;
    }
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        tmp_0 = packet.lookahead<bit<64>>();
        transition select(tmp_0[63:0]) {
            64w0: parse_cpu_header;
            default: parse_ethernet;
        }
    }
}

struct tuple_0 {
    standard_metadata_t field;
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name("NoAction") action NoAction_0() {
    }
    @name(".do_copy_to_cpu") action do_copy_to_cpu_0() {
        clone3<tuple_0>(CloneType.I2E, 32w250, { meta.standard_metadata });
    }
    @name(".copy_to_cpu") table copy_to_cpu {
        actions = {
            do_copy_to_cpu_0();
            @defaultonly NoAction_0();
        }
        size = 1;
        default_action = NoAction_0();
    }
    apply {
        copy_to_cpu.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<cpu_header_t>(hdr.cpu_header);
        packet.emit<ethernet_t>(hdr.ethernet);
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
