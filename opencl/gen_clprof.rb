require 'erb'
require 'yaml'

if ENV["SRC_DIR"]
  SRC_DIR = ENV["SRC_DIR"]
else
  SRC_DIR = "."
end

opencl_model = YAML::load_file("opencl_model.yaml")

SUFFIXES = opencl_model["suffixes"]
START = SUFFIXES["start"]
STOP = SUFFIXES["stop"]

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
        @name_unsanitized[/:(.*?)_?(?:#{START}|#{STOP})?$/,1]
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
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,0,'"clEnqueueReadBuffer"'], 10] ]
    },
    {"name" => "profiling_inversed",
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,0,'"clEnqueueWriteBuffer"'], 10],
                             [ ['"aurora12.gov"',-1,2,0,0,'"clEnqueueReadBuffer"'], 5] ]
    },
    {"name" => "profiling_block",
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,0,'"clEnqueueReadBuffer"'], 10 ]  ]
    },
    {"name" => "profiling_fast",
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,0,'"clEnqueueReadBuffer"'], 10]  ]
    },
    {"name" => "profiling_interleave_thread",
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,0,'"clEnqueueReadBuffer"'], 10],
                [ ['"aurora12.gov"',-1,3,0,0,'"clEnqueueWriteBuffer"'], 20] ]
    },
    {"name" => "profiling_interleave_process",
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,0,'"clEnqueueReadBuffer"'], 10],
                [ ['"aurora12.gov"', 1,2,0,0,'"clEnqueueWriteBuffer"'], 20] ]
    },
    {"name" => "profiling_normal_command_queue",
     "device_id_result" => [ [ ['"aurora12.gov"',0,0,0,10,'"clEnqueueReadBuffer"'], 65] ]
    },
    {"name" => "profiling_with_error",
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,10,'"clEnqueueWriteBuffer"'], 20] ]
    },
    {"name" => "profiling_normal_command_queue_created_in_other_thread",
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,10,'"clEnqueueReadBuffer"'], 10] ]
    },
    {"name" => "device_name",
     "device_to_name" => [ [ ['"aurora12.gov"', 11, 22], '"PVC"'] ]
    },
    {"name" => "kernel_name",
     "kernel_to_name" => [ [ ['"aurora24.gov"',666, 12], '"__ompoffload"'] ]
    },
    {"name" => "profiling_normal_nd_range_kernel_name",
     "device_id_result" => [ [ ['"aurora12.gov"',-1,2,0,0,'"__ompoffload"'], 10] ]
    },
    { "name" => "API_call",
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
        "#{name}.dust"
    end

end

$dbt_events = opencl_model['events'].map { |dbt_event|
        name_unsanitized, fields = dbt_event
        DBT_event.new(name_unsanitized, fields)
}

def write_file_via_template(file, testing = false)
    template = File.read(File.join(SRC_DIR, "#{file}.erb"))
    template_rendered = ERB.new(template).result(binding).gsub(/^\s*$\n/,'')
    if testing
        File.write("testing_#{file}", template_rendered)
    else
        File.write("#{file}", template_rendered)
    end
end

$sink_type = ARGV[0]
$l_file_generated=['clprof_callbacks.cpp','clprof_callbacks.h','clprof.c']
$l_test = l_test_d.map{ |  d | Test_clprof.new(d) }

if $sink_type == 'dust'
    write_file_via_template('dust.c')
elsif $sink_type == 'production' or $sink_type == 'testing'
    $l_file_generated.each{ |f|
        write_file_via_template(f, $sink_type == 'testing' ) 
    }
elsif $sink_type == 'testing_makefile'
    write_file_via_template('testing.mk')
end
