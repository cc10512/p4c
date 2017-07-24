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
}

struct __headersImpl {
    @name("cpu_header") 
    cpu_header_t cpu_header;
    @name("ethernet") 
    ethernet_t   ethernet;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    bit<64> tmp;
    @name(".parse_cpu_header") state parse_cpu_header {
        packet.extract<cpu_header_t>(hdr.cpu_header);
        transition parse_ethernet;
    }
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        tmp = packet.lookahead<bit<64>>();
        transition select(tmp[63:0]) {
            64w0: parse_cpu_header;
            default: parse_ethernet;
        }
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name("._drop") action _drop_0() {
        mark_to_drop();
    }
    @name(".do_cpu_encap") action do_cpu_encap_0() {
        hdr.cpu_header.setValid();
        hdr.cpu_header.device = 8w0;
        hdr.cpu_header.reason = 8w0xab;
    }
    @name(".redirect") table redirect_0 {
        actions = {
            _drop_0();
            do_cpu_encap_0();
            @defaultonly NoAction();
        }
        key = {
            standard_metadata.instance_type: exact @name("standard_metadata.instance_type") ;
        }
        size = 16;
        default_action = NoAction();
    }
    apply {
        redirect_0.apply();
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".do_copy_to_cpu") action do_copy_to_cpu_0() {
        clone3<tuple<standard_metadata_t>>(CloneType.I2E, 32w250, { standard_metadata });
    }
    @name(".copy_to_cpu") table copy_to_cpu_0 {
        actions = {
            do_copy_to_cpu_0();
            @defaultonly NoAction();
        }
        size = 1;
        default_action = NoAction();
    }
    apply {
        copy_to_cpu_0.apply();
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

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), egress(), __computeChecksumImpl(), __DeparserImpl()) main;
