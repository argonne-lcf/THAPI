#!/usr/bin/env ruby
# Generate a merged upstream YAML model for the MPI interval filter.
#
# btx_filter_mpi needs to receive Level Zero allocation/free events in
# addition to MPI events so that btx_mpiinterval_callbacks.cpp can
# maintain an address-range map and tag GPU-aware MPI calls. Two
# constraints conspire to make this non-trivial:
#
#  1. metababel's bare `-u model_a,model_b` union fails because each
#     backend model declares :environment: :entries:[{name: hostname}],
#     causing the matching engine to see an ambiguous member.
#
#  2. The MPI bt2 plugin (libMPIInterval) is built with only mpi.h on
#     the include path; pulling in the full ze event payloads would
#     drag in <ze_api.h> types (ze_context_handle_t, ze_result_t,
#     ze_device_mem_alloc_desc_t *, ...) that the MPI consumer cannot
#     resolve.
#
# Side-step both by emitting a single merged model that:
#   - starts from btx_mpi_model.yaml (single :environment:),
#   - copies only the ze allocation/free event classes we need into
#     MPI's stream class,
#   - keeps only the payload fields the MPI consumer actually uses,
#     so all C types referenced in the generated upstream typedefs
#     remain primitive (size_t / void *) and need no ze headers.
#
# Usage: ruby gen_btx_mpi_with_ze_model.rb MPI_MODEL.yaml ZE_MODEL.yaml > OUT.yaml
require 'yaml'

mpi_path = ARGV[0]
ze_path  = ARGV[1]
raise 'usage: gen_btx_mpi_with_ze_model.rb MPI_MODEL ZE_MODEL' unless mpi_path && ze_path

# (event name) -> (allow-list of payload field names the MPI consumer
# needs to see). Fields not in the allow-list are dropped from the
# copied event, keeping the generated upstream typedefs free of any
# ze-specific types.
ZE_EVENT_FIELDS = {
  'lttng_ust_ze:zeMemAllocHost_entry'    => %w[size],
  'lttng_ust_ze:zeMemAllocHost_exit'     => %w[pptr_val],
  'lttng_ust_ze:zeMemAllocDevice_entry'  => %w[size],
  'lttng_ust_ze:zeMemAllocDevice_exit'   => %w[pptr_val],
  'lttng_ust_ze:zeMemAllocShared_entry'  => %w[size],
  'lttng_ust_ze:zeMemAllocShared_exit'   => %w[pptr_val],
  'lttng_ust_ze:zeMemFree_entry'         => %w[ptr],
  'lttng_ust_ze:zeMemFree_exit'          => %w[],
}.freeze

mpi_model = YAML.load_file(mpi_path)
ze_model  = YAML.load_file(ze_path)

ze_events = ze_model.fetch(:stream_classes, []).flat_map do |sc|
  sc.fetch(:event_classes, []).map do |ec|
    wanted = ZE_EVENT_FIELDS[ec[:name]]
    next nil unless wanted

    # Deep-copy via Marshal so we can mutate freely without aliasing the
    # original ze model.
    ec = Marshal.load(Marshal.dump(ec))
    if (pfc = ec[:payload_field_class]) && (members = pfc[:members])
      pfc[:members] = members.select { |m| wanted.include?(m[:name].to_s) }
      ec.delete(:payload_field_class) if pfc[:members].empty?
    end
    ec
  end.compact
end

if ze_events.empty?
  warn "WARNING: no ze allocation events found in #{ze_path}; " \
       'GPU-aware MPI tagging will be inactive.'
end

# Inject the ze events into the MPI stream class. There is only one
# stream class per backend model (see utils/gen_babeltrace_model_helper.rb).
mpi_model[:stream_classes].first[:event_classes].concat(ze_events)

puts mpi_model.to_yaml
