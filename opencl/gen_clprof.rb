require 'erb'
require 'yaml'
opencl_model = YAML::load_file("opencl_model.yaml")

# Todo. This list is not complete
$cl_type_to_bl_type = {
"cl_platform_id" => "integer_unsigned",
"cl_device_id" => "integer_unsigned",
"cl_context" => "integer_unsigned",
"cl_command_queue"  => "integer_unsigned",
"cl_mem"  => "integer_unsigned",
"cl_program" => "integer_unsigned",
"cl_kernel" => "integer_unsigned",
"cl_event" => "integer_unsigned",
"cl_sampler" => "integer_unsigned",
"cl_GLsync" => "integer_unsigned",
"CLeglImageKHR" => "integer_unsigned",
"CLeglDisplayKHR" => "integer_unsigned",
"CLeglSyncKHR" => "integer_unsigned",
"cl_accelerator_intel" => "integer_unsigned",
"unsigned int" => "integer_unsigned",
"int" => "integer_signed",
"intptr_t" => "integer_signed",
"uintptr_t" => "integer_unsigned",
"size_t" => "integer_unsigned",
"cl_int" => "integer_signed",
"cl_uint" => "integer_unsigned",
"cl_long" => "integer_signed",
"cl_ulong" => "integer_unsigned",
"cl_short" => "integer_signed",
"cl_ushort" => "integer_unsigned",
"cl_char" => "string",
"cl_uchar" => "string",
"cl_half" => "real_single",
"cl_float" => "real_single",
"cl_double" => "real_double",
'cl_errcode' => 'integer_signed',
'string' => 'string'
}

class DBT_event

    def initialize(name_unsanitized, fields)
        @name_unsanitized = name_unsanitized
        @fields = fields
    end

    def name_unsanitized
        @name_unsanitized
    end
    
    def fields
        @fields
    end

    def name
        @name_unsanitized.gsub(':','_')
    end

    def name_striped
        @name_unsanitized[/:(.*?)_?(?:start|stop)?$/,1]
    end

    Payload = Struct.new(:lttng_type,:name)

    def payloads

        @fields.map { |name, f_info|
            cl_type = f_info["type"]
            l = Payload.new($cl_type_to_bl_type.fetch(cl_type,'string'),name)
            if f_info['array']
              [Payload.new('integer_unsigned',"_#{name}_length"),l]
            else
              l
            end
        }.flatten
    end

    def callback_signature
        head = ['const bt_event *bt_evt', 'const bt_clock_snapshot *bt_clock']
        l = @fields.map { | name, f_info|
             t = f_info['type'].gsub('cl_errcode','cl_int')
             if f_info['pointer'] or f_info['array'] or f_info['string']
                 t = "#{t} *"
             end
             if f_info['array']
                 ["size_t _#{name}_length", "#{t} #{name}"]
             else
                ["#{t} #{name}"]
             end
        }.flatten

        a = head+l
        a.join(",\n")
    end
end

l_test_d = [
    {"name" => "profiling_normal",
    "dust"=> [
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_start'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling', ['event=12'] ],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_stop'],
               ['aurora12.gov',-1,3, 'lttng_ust_opencl_profiling:event_profiling_results', ['event=12'], ['start=0'], ['end=10'] ] ],
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,'"clEnqueueReadBuffer"'], 10] ]
    },
    {"name" => "profiling_inversed",
    "dust"=> [
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueWriteBuffer_start'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling_results', ['event=12'], ['start=0'], ['end=10'] ],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling', ['event=12'] ],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueWriteBuffer_stop'],

               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_start'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling', ['event=12'] ],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_stop'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling_results', ['event=12'], ['start=0'], ['end=5'] ] ],
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,'"clEnqueueWriteBuffer"'], 10],
                             [ ['"aurora12.gov"',-1,2,0,'"clEnqueueReadBuffer"'], 5] ]
    },
    {"name" => "profiling_block",
    "dust"=> [ ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_start'],
               ['aurora12.gov',-1,3, 'lttng_ust_opencl_profiling:event_profiling_results', ['event=12'], ['start=0'], ['end=10'] ],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling', ['event=12'] ],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_stop'] ],
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,'"clEnqueueReadBuffer"'], 10 ]  ]
    },
    {"name" => "profiling_fast",
    "dust"=> [ ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_start'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling', ['event=12'] ],
               ['aurora12.gov',-1,3, 'lttng_ust_opencl_profiling:event_profiling_results', ['event=12'], ['start=0'], ['end=10'] ],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_stop'] ],
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,'"clEnqueueReadBuffer"'], 10]  ]
    },
    {"name" => "profiling_interleave_thread",
    "dust"=> [ ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_start'],
               ['aurora12.gov',-1,3, 'lttng_ust_opencl:clEnqueueWriteBuffer_start'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling', ['event=12'] ],
               ['aurora12.gov',-1,3, 'lttng_ust_opencl_profiling:event_profiling', ['event=20'] ],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_stop'],
               ['aurora12.gov',-1,3, 'lttng_ust_opencl:clEnqueueWriteBuffer_stop'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling_results', ['event=12'], ['start=0'], ['end=10'] ],
               ['aurora12.gov',-1,3, 'lttng_ust_opencl_profiling:event_profiling_results', ['event=20'], ['start=0'], ['end=20'] ] ],
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,'"clEnqueueReadBuffer"'], 10],
                [ ['"aurora12.gov"',-1,3,0,'"clEnqueueWriteBuffer"'], 20] ]
    },
    {"name" => "profiling_interleave_process",
    "dust"=> [ ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_start'],
               ['aurora12.gov',' 1',2, 'lttng_ust_opencl:clEnqueueWriteBuffer_start'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling', ['event=12'] ],
               ['aurora12.gov',' 1',2, 'lttng_ust_opencl_profiling:event_profiling', ['event=12'] ],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_stop'],
               ['aurora12.gov',' 1',2, 'lttng_ust_opencl:clEnqueueWriteBuffer_stop'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling_results', ['event=12'], ['start=0'], ['end=10'] ],
               ['aurora12.gov',' 1',2, 'lttng_ust_opencl_profiling:event_profiling_results', ['event=12'], ['start=0'], ['end=20'] ] ],
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,'"clEnqueueReadBuffer"'], 10],
                [ ['"aurora12.gov"', 1,2,0,'"clEnqueueWriteBuffer"'], 20] ]
    },
    {"name" => "profiling_normal_command_queue",
    "dust"=> [ ['aurora12.gov',-1,2, 'lttng_ust_opencl:clCreateCommandQueue_start','device=10'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clCreateCommandQueue_stop', 'command_queue=23'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_start', 'command_queue=23'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling', ['event=12'] ],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_stop'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling_results', ['event=12'], ['start=0'], ['end=10'] ] ],
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,10,'"clEnqueueReadBuffer"'], 10] ]
    },
    {"name" => "profiling_normal_command_queue_created_in_other_thread",
    "dust"=> [ ['aurora12.gov',-1,3, 'lttng_ust_opencl:clCreateCommandQueue_start','device=10'],
               ['aurora12.gov',-1,3, 'lttng_ust_opencl:clCreateCommandQueue_stop', 'command_queue=23'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_start', 'command_queue=23'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling', ['event=12'] ],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueReadBuffer_stop'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling_results', ['event=12'], ['start=0'], ['end=10'] ] ],
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,10,'"clEnqueueReadBuffer"'], 10] ]
    },
    {"name" => "device_name",
     "dust"=> [ ['aurora12.gov',11,0, 'lttng_ust_opencl_devices:device_name', 'device=22', 'name=PVC'] ],
     "device_to_name" => [ [ ['"aurora12.gov"', 11, 22], '"PVC"'] ]
    },
    {"name" => "kernel_name",
     "dust"=> [ ['aurora24.gov',666,0, 'lttng_ust_opencl_arguments:kernel_info', 'kernel=12', 'function_name=__ompoffload'] ],
     "kernel_to_name" => [ [ ['"aurora24.gov"',666, 12], '"__ompoffload"'] ]
    },
    {"name" => "profiling_normal_nd_range_kernel_name",
    "dust"=> [ ['aurora24.gov',-1,3, 'lttng_ust_opencl_arguments:kernel_info', 'function_name=__ompoffload','kernel=12'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueNDRangeKernel_start','kernel=12'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling', ['event=12'] ],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl:clEnqueueNDRangeKernel_stop'],
               ['aurora12.gov',-1,2, 'lttng_ust_opencl_profiling:event_profiling_results', ['event=12'], ['start=0'], ['end=10'] ] ],
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,'"__ompoffload"'], 10] ]
    },
    { "name" => "API_call",
      "dust" => [ ['aurora12.gov',-1,2, 'lttng_ust_opencl:clLinkProgram_start'],
                  ['aurora12.gov',-1,2, 'lttng_ust_opencl:clLinkProgram_stop'],
                  ['aurora12.gov',-1,2, 'lttng_ust_opencl:clLinkProgram_start'],
                  ['aurora12.gov',-1,2, 'lttng_ust_opencl:clLinkProgram_stop'] ],
     "api_call" => [ [ ['"aurora12.gov"',-1,2, '"clLinkProgram"'], [2,1,1,1.0]  ] ]
    }
]

class Test_clprof
    def initialize(d)
        @d =  d
    end

    def dust
        @d['dust'].each_with_index.map{ |l,i| "#{i} #{l.push('foo').join(' ')} \n"}.join
    end

    def api_call
        @d.fetch('api_call', [] )
    end

    def device_id_result
        @d.fetch('device_id_result',[])
    end 

    def device_to_name
        @d.fetch('device_to_name',[])
    end 

    def kernel_to_name
        @d.fetch('kernel_to_name',[])
    end

    def name
        @d['name']
    end

    def path
        "test_clprof/#{name}.dust"
    end

end

$dbt_events = opencl_model['events'].map { |dbt_event|
        name_unsanitized, fields = dbt_event
        DBT_event.new(name_unsanitized, fields)
}

def write_file_via_template(file, testing = false)
    template = File.read("#{file}.erb")
    template_rendered = ERB.new(template).result(binding).gsub(/^\s*$\n/,'')
    if testing
        File.write("testing_#{file}", template_rendered)
    else
        File.write("#{file}", template_rendered)
    end
end

$sink_type = ARGV[0]
$l_file_generated=['clprof_callbacks.cpp','clprof_callbacks.h','clprof.c','utils.h']
$l_test = l_test_d.map{ |  d | Test_clprof.new(d) }

if $sink_type == 'dust'
    write_file_via_template('dust.c')
elsif $sink_type == 'production' or $sink_type == 'testing'
    if $sink_type == 'testing'
       $l_test.each{ | test | File.write(test.path, test.dust) }
    end
    $l_file_generated.each{ |f|
        write_file_via_template(f, $sink_type == 'testing' ) 
    }
elsif $sink_type == 'testing_makefile'
    write_file_via_template('testing.mk')
end
