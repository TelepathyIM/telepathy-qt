# Input for tools/manager-file.py

MANAGER = 'example_contact_list'
PARAMS = {
        'example' : {
            'account': {
                'dtype': 's',
                'flags': 'required register',
                'filter': 'account_param_filter',
                # 'filter_data': 'NULL',
                # 'default': ...,
                # 'struct_field': '...',
                # 'setter_data': 'NULL',
                },
            'simulation-delay': {
                'dtype': 'u',
                'default': 1000,
                },
            },
        }
STRUCTS = {
        'example': 'ExampleParams'
        }
