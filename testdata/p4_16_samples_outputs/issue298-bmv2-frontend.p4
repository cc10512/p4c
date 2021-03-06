#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     totalLen;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     fragOffset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdrChecksum;
    IPv4Address srcAddr;
    IPv4Address dstAddr;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> length_;
    bit<16> checksum;
}

header myhdr_t {
    bit<16> msgtype;
    bit<32> inst;
    bit<16> rnd;
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
    @name("ipv4") 
    ipv4_t     ipv4;
    @name("udp") 
    udp_t      udp;
    @name("myhdr") 
    myhdr_t    myhdr;
}

struct ingress_metadata_t {
    bit<16> round;
    bit<1>  set_drop;
}

struct metadata {
    @name("ingress_metadata") 
    ingress_metadata_t local_metadata;
}

parser TopParser(packet_in b, out headers p, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        b.extract<ethernet_t>(p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x800: parse_ipv4;
        }
    }
    state parse_ipv4 {
        b.extract<ipv4_t>(p.ipv4);
        transition select(p.ipv4.protocol) {
            8w0x11: parse_udp;
            default: accept;
        }
    }
    state parse_udp {
        b.extract<udp_t>(p.udp);
        transition select(p.udp.dstPort) {
            16w0x8888: parse_myhdr;
            default: accept;
        }
    }
    state parse_myhdr {
        b.extract<myhdr_t>(p.myhdr);
        transition accept;
    }
}

control TopDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<udp_t>(hdr.udp);
        packet.emit<myhdr_t>(hdr.myhdr);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta) {
    bit<16> tmp;
    bool tmp_0;
    @name("ipv4_checksum") Checksum16() ipv4_checksum_0;
    apply {
        tmp = ipv4_checksum_0.get<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
        tmp_0 = hdr.ipv4.hdrChecksum == tmp;
        if (tmp_0) 
            mark_to_drop();
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    bit<16> tmp_1;
    @name("ipv4_checksum") Checksum16() ipv4_checksum_1;
    apply {
        tmp_1 = ipv4_checksum_1.get<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>>({ hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr });
        hdr.ipv4.hdrChecksum = tmp_1;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("_drop") action _drop_0() {
        mark_to_drop();
    }
    @name("drop_tbl") table drop_tbl_0 {
        key = {
            meta.local_metadata.set_drop: exact @name("meta.ingress_metadata.set_drop") ;
        }
        actions = {
            _drop_0();
            NoAction();
        }
        size = 2;
        default_action = NoAction();
    }
    apply {
        drop_tbl_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("registerRound") register<bit<16>>(32w65536) registerRound_0;
    @name("read_round") action read_round_0() {
        registerRound_0.read(meta.local_metadata.round, hdr.myhdr.inst);
    }
    @name("round_tbl") table round_tbl_0 {
        key = {
        }
        actions = {
            read_round_0();
        }
        size = 8;
        default_action = read_round_0();
    }
    apply {
        if (hdr.ipv4.isValid()) 
            if (hdr.myhdr.isValid()) 
                round_tbl_0.apply();
    }
}

V1Switch<headers, metadata>(TopParser(), verifyChecksum(), ingress(), egress(), computeChecksum(), TopDeparser()) main;
