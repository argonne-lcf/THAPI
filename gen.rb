require_relative 'opencl_model'

INSTR = :lttng

provider = :lttng_ust_opencl

puts <<EOF
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/opencl.h>
#include <CL/cl_gl_ext.h>
#include <CL/cl_egl.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
EOF

if INSTR == :lttng
  puts '#include "opencl_tracepoints.h"'
  puts '#include "opencl_profiling.h"'
  puts '#include "opencl_source.h"'
end

$opencl_commands.each { |c|
  puts <<EOF
static #{c.decl_pointer} = (void *) 0x0;
EOF
}

$clReleaseEvent = $opencl_commands.find { |c| c.prototype.name == "clReleaseEvent" }
$clSetEventCallback = $opencl_commands.find { |c| c.prototype.name == "clSetEventCallback" }
$clGetEventProfilingInfo = $opencl_commands.find { |c| c.prototype.name == "clGetEventProfilingInfo" }

puts <<EOF
void CL_CALLBACK  event_notify (cl_event event, cl_int event_command_exec_status, void *user_data) {
  if (tracepoint_enabled(#{provider}_profiling, event_profiling_results)) {
    cl_ulong queued;
    cl_ulong submit;
    cl_ulong start;
    cl_ulong end;
    cl_int queued_status = #{$clGetEventProfilingInfo.prototype.pointer_name}(event, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &queued, NULL);
    cl_int submit_status = #{$clGetEventProfilingInfo.prototype.pointer_name}(event, CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &submit, NULL);
    cl_int start_status = #{$clGetEventProfilingInfo.prototype.pointer_name}(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    cl_int end_status = #{$clGetEventProfilingInfo.prototype.pointer_name}(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    do_tracepoint(#{provider}_profiling, event_profiling_results, event, event_command_exec_status,
              queued_status, queued, submit_status, submit, start_status, start, end_status, end);
  }
}

void __load_tracer(void) __attribute__((constructor));
void __load_tracer(void) {
  void * handle = dlopen("libOpenCL.so", RTLD_LAZY | RTLD_LOCAL);
  if( !handle ) {
    printf("Failure: could not load OpenCL library!\\n");
    exit(1);
  }
EOF

$opencl_commands.each { |c|
  puts <<EOF
  #{c.prototype.pointer_name} = dlsym(handle, "#{c.prototype.name}") ;
EOF
}

puts <<EOF
}
EOF

$opencl_commands.each { |c|
  puts <<EOF
#{c.decl} {
EOF
  params = []
  params = c.parameters.collect(&:name) unless c.parameters.size == 1 && c.parameters.first.decl.strip == "void"
  if INSTR == :printf
    puts '  printf("Called: #{c.prototype.name}\\n");'
  elsif INSTR == :lttng && c.parameters.length <= LTTNG_USABLE_PARAMS
    tracepoint_params = c.tracepoint_parameters.collect(&:name)
    c.tracepoint_parameters.each { |p|
      puts "  #{p.type} #{p.name};"
    }
    c.tracepoint_parameters.each { |p|
      puts p.init
    }
    puts "  tracepoint(#{provider}, #{c.prototype.name}_start, #{(params+tracepoint_params).join(", ")});"
    if c.prototype.name == "clCreateCommandQueue"
      puts <<EOF
  if (tracepoint_enabled(#{provider}_profiling, event_profiling)) {
    properties |= CL_QUEUE_PROFILING_ENABLE;
  }
EOF
    end
    if c.prototype.name == "clCreateCommandQueueWithProperties"
      puts <<EOF
  cl_queue_properties *_profiling_properties = NULL;
  if (tracepoint_enabled(#{provider}_profiling, event_profiling)) {
    int _found_queue_properties = 0;
    int _queue_properties_index = 0;
    int _properties_count = 0;
    if (properties) {
      while(properties[_properties_count]) {
        if (properties[_properties_count] == CL_QUEUE_PROPERTIES){
          _found_queue_properties = 1;
          _queue_properties_index = _properties_count;
        }
        _properties_count += 2;
      }
      _properties_count++;
      if (!_found_queue_properties)
        _properties_count +=2;
    } else
      _properties_count = 3;
    _profiling_properties = (cl_queue_properties *)malloc(_properties_count*sizeof(cl_queue_properties));
    if (_profiling_properties) {
      if (properties) {
        int _i = 0;
        while(properties[_i]) {
          _profiling_properties[_i] = properties[_i];
          _profiling_properties[_i+1] = properties[_i+1];
          _i += 2;
        }
        if (_found_queue_properties) {
          _profiling_properties[_queue_properties_index+1] |= CL_QUEUE_PROFILING_ENABLE;
          _profiling_properties[_i] = 0;
        } else {
          _profiling_properties[_i++] = CL_QUEUE_PROPERTIES;
          _profiling_properties[_i++] = CL_QUEUE_PROFILING_ENABLE;
          _profiling_properties[_i] = 0;
        }
      } else {
        _profiling_properties[0] = CL_QUEUE_PROPERTIES;
        _profiling_properties[1] = CL_QUEUE_PROFILING_ENABLE;
        _profiling_properties[2] = 0;
      }
      properties = _profiling_properties;
    }
  }
EOF
    end
    if c.prototype.name == "clCreateProgramWithSource"
      puts <<EOF
  if (tracepoint_enabled(#{provider}_source, program_string) && strings != NULL) {
    int index;
    for (index = 0; index < count; index++) {
      size_t length = 0;
      if ( strings[index] != NULL ) {
        if (lengths == NULL || lengths[index] == 0)
          length = strlen(strings[index]);
        else
          length = lengths[index];
      }
      do_tracepoint(#{provider}_source, program_string, index, length, strings);
    }
  }
EOF
    end
    if c.prototype.name == "clCreateProgramWithBinary"
      puts <<EOF
  if (tracepoint_enabled(#{provider}_source, program_binary) && binaries != NULL && lengths != NULL) {
    int index;
    for (index = 0; index < num_devices; index++) {
      do_tracepoint(#{provider}_source, program_binary, index, lengths, binaries);
    }
  }
EOF
    end
    if c.prototype.name == "clCreateProgramWithIL"
      puts <<EOF
  if (tracepoint_enabled(#{provider}_source, program_il) && il != NULL) {
    do_tracepoint(#{provider}_source, program_il, length, il);
  }
EOF
    end
    if c.event? && !c.returns_event?
      event = c.parameters.find { |p| p.name == "event" && p.pointer? }
      puts <<EOF
  int _release_event = 0;
  int _event_profiling = 0;
  cl_event profiling_event;
  if (tracepoint_enabled(#{provider}_profiling, event_profiling) && #{event.name} == NULL) {
    #{event.name} = &profiling_event;
    _release_event = 1;
    _event_profiling = 1;
  }
EOF
    end
  else
    $stderr.puts "Skipped: #{c.prototype.name}"
  end
  if c.prototype.has_return_type?
    puts <<EOF
  #{c.prototype.return_type} _retval;
  _retval = #{c.prototype.pointer_name}(#{params.join(", ")});
EOF
    if c.prototype.name == "clCreateCommandQueueWithProperties"
      puts "  if (_profiling_properties) free(_profiling_properties);"
    end
    if INSTR == :lttng && c.parameters.length <= LTTNG_USABLE_PARAMS
      if c.event?
        if !c.returns_event?
          event = c.parameters.find { |p| p.name == "event" && p.pointer? }
          puts <<EOF
  if (_event_profiling) {
    int _set_retval = #{$clSetEventCallback.prototype.pointer_name}(*#{event.name}, CL_COMPLETE, event_notify, NULL);
    do_tracepoint(#{provider}_profiling, event_profiling, _set_retval, *#{event.name});
    if(_release_event) {
      #{$clReleaseEvent.prototype.pointer_name}(*#{event.name});
      #{event.name} = NULL;
    }
  }
EOF
        else
          puts <<EOF
  if (tracepoint_enabled(#{provider}_profiling, event_profiling) ) {
    int _set_retval = #{$clSetEventCallback.prototype.pointer_name}(_retval, CL_COMPLETE, event_notify, NULL);
    do_tracepoint(#{provider}_profiling, event_profiling, _set_retval, _retval);
  }
EOF
        end
      end
      params.push "_retval"
      puts "  tracepoint(#{provider}, #{c.prototype.name}_stop, #{(params+tracepoint_params).join(", ")});"
    end
    puts <<EOF
  return _retval;
}

EOF
  else
    puts "  #{c.prototype.pointer_name}(#{params.join(", ")});"
    if c.prototype.name == "clCreateCommandQueueWithProperties"
      puts "  if (_profiling_properties) free(_profiling_properties);"
    end
    if INSTR == :lttng && c.parameters.length <= LTTNG_USABLE_PARAMS
      puts "  tracepoint(#{provider}, #{c.prototype.name}_stop, #{(params+tracepoint_params).join(", ")});"
    end
    puts "}"
    puts
  end
}
