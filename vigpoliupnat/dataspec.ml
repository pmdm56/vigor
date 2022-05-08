open Data_spec
open Core
open Ir

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

let constraints = ["int_dev_bounds", ( "uint32_t",
                                         [Bop (Lt, {t=Unknown;v=Id "v"}, {t=Unknown;v=Int 2});
                                          Not {v=Bop (Eq, {t=Unknown;v=Id "v"}, {t=Unknown;v=Int 1});
                                               t=Unknown};
                                         ]);
                  "dyn_val_condition", ( "DynamicValue",
                                          [Bop (Le, {t=Unknown;v=Int 0}, {t=Unknown;v=Id "bucket_time"});
                                           Bop (Le, {t=Unknown;v=Id "bucket_time"}, {t=Unknown;v=Id "t"});
                                           Bop (Le, {t=Unknown;v=Id "bucket_size"}, {t=Unknown;v=Int 10000000000});
                                          ])]

let gen_custom_includes = ref []
let gen_records = ref []
