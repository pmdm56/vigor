open Core
open Str
open Fspec_api
open Ir
open Common_fspec

let flow_id_struct = Ir.Str ( "FlowId", ["src_port", Uint16;
                                         "dst_port", Uint16;
                                         "src_ip", Uint32;
                                         "dst_ip", Uint32;
                                         "protocol", Uint8;] )

let ip_addr_struct = Ir.Str ( "ip_addr", ["addr", Uint32;])
let dynamic_value_struct = Ir.Str ( "DynamicValue", ["bucket_size", Uint64;

module Iface : Fspec_api.Spec =
struct
let containers = ["fm", Map ("FlowId", "max_flows", "");
                  "fv", Vector ("FlowId", "max_flows", "");
                  "int_devices", Vector ("uint32_t", "max_flows", "int_dev_bounds");
                  "heap", DChain "max_flows";
                  "max_flows", Int;
                  "fw_device", UInt32;
                  "flow_emap", EMap ("FlowId", "fm", "fv", "heap");
                  "dyn_map", Map ("ip_addr", "capacity", "");
                  "dyn_keys", Vector ("ip_addr", "capacity", "");
                  "dyn_heap", DChain "capacity";
                  "dyn_vals", Vector ("DynamicValue", "capacity", "dyn_val_condition");
                  "capacity", UInt32;
                  "dev_count", UInt32;
                  "poli_flow_emap", EMap ("ip_addr", "dyn_map", "dyn_keys", "dyn_heap"); ]

let records = String.Map.of_alist_exn
    ["FlowId", flow_id_struct;
     "uint32_t", Uint32;
     "ip_addr", ip_addr_struct;
      "DynamicValue", dynamic_value_struct]
end

(* Register the module *)
let () =
  Fspec_api.spec := Some (module Iface) ;

