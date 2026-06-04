#!/usr/bin/env ruby
# Generate a merged upstream YAML model for the MPI interval filter.
#
# btx_filter_mpi needs to receive Level Zero allocation/free events in
# addition to MPI events so that btx_mpiinterval_callbacks.cpp can
# maintain an address-range map and tag GPU-aware MPI calls. metababel's
# -u union of two backend models fails because both declare
# :environment: :entries: [{name: hostname}], which the matching
# engine treats as an ambiguity.
#
# This generator side-steps the issue by emitting a single model file
# that:
#   - starts from btx_mpi_model.yaml,
#   - copies only the ze allocation/free event classes we actually
#     need into the same stream class.
#
# Usage: ruby gen_btx_mpi_with_ze_model.rb MPI_MODEL.yaml ZE_MODEL.yaml > OUT.yaml
require 'yaml'

mpi_path = ARGV[0]
ze_path  = ARGV[1]
raise 'usage: gen_btx_mpi_with_ze_model.rb MPI_MODEL ZE_MODEL' unless mpi_path && ze_path

ZE_WANTED_EVENT_NAMES = %w[
  lttng_ust_ze:zeMemAllocHost_entry
  lttng_ust_ze:zeMemAllocHost_exit
  lttng_ust_ze:zeMemAllocDevice_entry
  lttng_ust_ze:zeMemAllocDevice_exit
  lttng_ust_ze:zeMemAllocShared_entry
  lttng_ust_ze:zeMemAllocShared_exit
  lttng_ust_ze:zeMemFree_entry
  lttng_ust_ze:zeMemFree_exit
].freeze

mpi_model = YAML.load_file(mpi_path)
ze_model  = YAML.load_file(ze_path)

ze_events = ze_model.fetch(:stream_classes, []).flat_map do |sc|
  sc.fetch(:event_classes, []).select { |ec| ZE_WANTED_EVENT_NAMES.include?(ec[:name]) }
end

if ze_events.empty?
  warn "WARNING: no ze allocation events found in #{ze_path}; " \
       'GPU-aware MPI tagging will be inactive.'
end

# Inject the ze events into the MPI stream class. There is only one
# stream class per backend model (see utils/gen_babeltrace_model_helper.rb).
mpi_model[:stream_classes].first[:event_classes].concat(ze_events)

puts mpi_model.to_yaml
