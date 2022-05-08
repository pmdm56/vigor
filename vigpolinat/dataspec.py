objConstructors = {'flow_emap.get_key' : {'constructor' : 'FlowIdc',
                                          'type' : 'FlowIdi',
                                          'fields' : ['sp', 'dp', 'sip',
                                                      'dip', 'prot']},
                    'dyn_vals.get' : {'constructor' : 'DynamicValuec',
                                     'type' : 'DynamicValuei',
                                     'fields' : ['bucket_size', 'bucket_time']}}

typeConstructors = {'FlowIdc' : 'FlowIdi',
                    'emap' : 'emap<FlowIdi>',
                    'DynamicValuec' : 'DynamicValuei'}

stateObjects = {'flow_emap' : emap,
                'int_devices' : vector,
                'poli_flow_emap' : emap,
                'dyn_vals' : vector}
