# Input for tools/manager-file.py

MANAGER = 'example_echo'
PARAMS = {
        'example' : {
            'account': {
                'dtype': 's',
                'flags': 'required register',
                'filter': 'tp_cm_param_filter_string_nonempty',
                # 'filter_data': 'NULL',
                # 'default': ...,
                # 'struct_field': '...',
                # 'setter_data': 'NULL',
                },
            },
        }
STRUCTS = {
        'example': 'ExampleParams'
        }
