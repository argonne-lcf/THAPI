# frozen_string_literal: true

require 'set'

# Error taxonomy and reporting for ze_validator.
module ZEValidator

  # A node of the error tree.
  # Severity is inherited from the parent unless the node overrides it
  class ErrorNode
    attr_reader :id, :title, :parent, :children

    def initialize(id, title, parent: nil, severity: nil)
      @id = id
      @title = title
      @parent = parent
      @children = []
      @severity = severity
    end

    # :error or :warning, walking up to the root until a node states one.
    def severity
      @severity || @parent&.severity || :error
    end

    def add_child(node)
      @children << node
      node
    end

    def root?
      @parent.nil?
    end

    def leaf?
      @children.empty?
    end

    # The root exists only to hold the categories together and carries no information
    def ancestors
      node = @parent
      out = []
      while node && !node.root?
        out.unshift(node)
        node = node.parent
      end
      out
    end

    # e.g. "MV/portability/command_queue_group_not_queried"
    def path
      (ancestors + [self]).map(&:id).join('/')
    end

    def depth
      ancestors.size
    end

    def each_node(&block)
      yield self unless root?
      @children.each { |c| c.each_node(&block) }
      self
    end

    def each_leaf(&block)
      each_node { |n| block.call(n) if n.leaf? }
    end
  end

  # Builds the tree from a nested declaration and indexes its leaves by id.
  class ErrorTreeBuilder
    attr_reader :root, :index

    def initialize(id, title)
      @root = ErrorNode.new(id, title)
      @index = {}
      @stack = [@root]
    end

    def category(id, title, severity: nil)
      node = @stack.last.add_child(ErrorNode.new(id, title, parent: @stack.last, severity: severity))
      @stack.push(node)
      yield if block_given?
      @stack.pop
      node
    end

    def leaf(id, title, severity: nil)
      raise "duplicate diagnostic id #{id.inspect}" if @index.key?(id)
      node = @stack.last.add_child(ErrorNode.new(id, title, parent: @stack.last, severity: severity))
      @index[id] = node
    end
  end

  # Level Zero error tree
  module ErrorTree
    def self.build
      b = ErrorTreeBuilder.new(:ze, 'Level Zero API usage findings')

      b.category(:PV, 'Progression Violations') do
        b.leaf :circular_event_dependency, 'circular event dependency between commands'
        b.leaf :in_order_self_deadlock,    'in-order list waits on an event it only signals later'
        b.leaf :unsignaled_wait_event,     'command waits on an event that is never signaled'
      end

      b.category(:MSV, 'Memory Safety Violations') do
        b.leaf :out_of_bounds_copy,        'copy or fill reaches past the end of its allocation'
        b.leaf :use_after_free,            'copy or barrier references a freed allocation'
        b.leaf :free_while_in_flight,      'allocation freed while in-flight device work still uses it'
        b.leaf :null_copy_pointer,         'copy or fill endpoint is a null pointer'
        b.leaf :unallocated_address_range, 'address queried was never allocated or is out of range'
        b.leaf :overlapping_allocation,    'allocator returned a range overlapping a live allocation'
      end

      # The edge letters refer to Figure 7 of the paper, which enumerates the
      # object pairs subject to a common-context requirement.
      b.category(:HMV, 'Hierarchy-Membership Violations') do
        b.leaf :fence_queue_mismatch,          'fence submitted to a queue other than the one it was created on (edge A)'
        b.leaf :list_queue_context_mismatch,   'command list and command queue on different contexts (edge B)'
        b.leaf :list_fence_context_mismatch,   'command list and fence on different contexts (edge A/B)'
        b.leaf :memory_list_context_mismatch,  'copied memory and command list on different contexts (edge C)'
        b.leaf :kernel_list_context_mismatch,  'kernel module and command list on different contexts (edge D)'
        b.leaf :event_list_context_mismatch,   'event pool and command list on different contexts (edge E)'
      end

      b.category(:MV, 'Miscellaneous Violations') do
        b.category(:engine, 'engine and ordinal binding') do
          b.leaf :kernel_on_copy_only_list,        'kernel appended to a list bound to a copy-only engine'
          b.leaf :compute_list_on_copy_only_queue, 'list holding a kernel submitted to a copy-only queue'
          b.leaf :queue_index_out_of_range,        'queue index outside the engine group it was created on'
        end

        b.category(:lifetime, 'object lifetime') do
          b.leaf :object_leak,            'object never destroyed'
          b.leaf :object_outlives_owner,  'object not destroyed before the object that owns it'
        end

        b.category(:object_state, 'object state') do
          b.leaf :command_list_not_closed,          'command list submitted without being closed'
          b.leaf :command_list_already_destroyed,   'destroyed command list submitted'
          b.leaf :command_list_reset_after_destroy, 'destroyed command list reset'
          b.leaf :command_list_reset_immediate,     'immediate command list reset'
          b.leaf :command_list_reset_in_flight,     'command list reset while a submission is still in flight'
          b.leaf :fence_reuse_without_reset,        'fence reused without being reset'
          b.leaf :event_reuse_without_reset,        'event reused as a signal target without being reset'
          b.leaf :event_concurrent_signal,          'event signaled again before being reset or consumed'
          b.leaf :event_pool_index_in_use,          'event pool index already in use'
          b.leaf :event_pool_index_already_free,    'event pool index already free'
        end

        b.category(:invalid_argument, 'invalid argument') do
          b.leaf :unknown_context,           'context handle was never created, or was already destroyed'
          b.leaf :unknown_command_queue,     'command queue handle was never created'
          b.leaf :no_command_list_submitted, 'no command list submitted'
          b.leaf :unknown_command_list,      'command list handle was never created'
          b.leaf :immediate_list_submitted,  'immediate command list submitted to a command queue'
          b.leaf :kernel_not_created,        'kernel handle was never created'
          b.leaf :null_module_handle,        'null module handle'
          b.leaf :null_fence_handle,         'null fence handle'
          b.leaf :null_event_pool_handle,    'null event pool handle'
        end

        b.category(:concurrency, 'concurrency') do
          b.leaf :concurrent_object_access, 'concurrent access to an object that is not thread-safe'
        end

        b.category(:api_conformance, 'API conformance') do
          b.leaf :descriptor_stype_mismatch, 'descriptor carries the wrong stype'
          b.leaf :init_not_called,           'API called before zeInit or zeInitDrivers'
          b.leaf :api_never_returned,        'API call never returned'
          b.leaf :deprecated_api,            'deprecated API used', severity: :warning
        end

        b.category(:portability, 'portability', severity: :warning) do
          b.leaf :command_queue_group_not_queried, 'command queue group never queried, ordinals are hardcoded'
        end

        # Not an error of the program, but an error from the tracing itself: it is missing ordinals of copy-engines.
        # In such case, report a warning
        b.category(:coverage, 'analysis coverage', severity: :warning) do
          b.leaf :engine_topology_unknown, 'copy-engine checks skipped, no engine topology in the trace'
        end
      end

      b
    end

    BUILDER = build
    ROOT = BUILDER.root
    LEAVES = BUILDER.index.freeze

    def self.[](id)
      LEAVES.fetch(id) { raise ArgumentError, "unknown diagnostic id #{id.inspect}" }
    end
  end

  # Formats findings, limits how often each kind may be printed, and keeps the
  # counts the end-of-trace summary reports.
  class Reporter
    # Printed lines allowed per diagnostic kind before the rest are suppressed.
    # A single kind routinely accounts for tens of thousands of findings in a
    # production trace, which drowns everything else. Edit here to change it.
    MAX_REPORTS_PER_KIND = 5

    TOOL = 'ze_validator'

    # Width of the dotted leader in the summary, chosen so the deepest label
    # still leaves room for the count.
    SUMMARY_WIDTH = 66

    def initialize(out: $stderr)
      @out = out
      # leaf -> every detection, including those suppressed below. This is what
      # the summary reports: it answers "how much of this is there", which the
      # printed lines no longer do once a kind is capped.
      @counts = Hash.new(0)
      # leaf -> lines actually printed, compared against MAX_REPORTS_PER_KIND
      @printed = Hash.new(0)
      # dedup keys already reported, for checks that would otherwise repeat
      # verbatim on the same object
      @seen_keys = Set.new
      # leaves whose suppression notice has been printed
      @capped = Set.new
    end

    # Reports one error.
    #
    # id       error id (leaf in the tree)
    # location the location fields, outermost first: [host, pid, tid] for a
    #          finding attributable to one call, [host, pid] for a process-wide
    # message  the human-readable description
    # key      optional dedup key; a finding whose key was already reported is
    #          counted but not printed, avoiding redundant reports
    def report(id, location, message, key: nil)
      node = ErrorTree[id]
      @counts[node] += 1

      return if key && !@seen_keys.add?("#{id}\u0000#{key}")
      return if capped?(node)

      @printed[node] += 1
      emit(node, location, message)
    end

    def error_count
      total_for(:error)
    end

    def warning_count
      total_for(:warning)
    end

    # Prints the per-kind error count
    def summary
      total = @counts.values.sum
      @out.puts
      @out.puts "===== #{TOOL} summary ====="
      if total.zero?
        @out.puts 'no findings'
        @out.puts '=' * (12 + TOOL.length)
        return
      end

      @out.puts format('%d %s: %d %s, %d %s, over %d %s',
                       total, plural(total, 'finding'),
                       error_count, plural(error_count, 'error'),
                       warning_count, plural(warning_count, 'warning'),
                       @counts.size, plural(@counts.size, 'kind'))
      @out.puts
      visible_children(ErrorTree::ROOT).each_with_index do |category, i|
        @out.puts if i.positive?
        print_summary_line(category, '')
        print_subtree(category, '')
      end
      @out.puts '=' * (12 + TOOL.length)
    end

    # Writes the per-kind counts as CSV.
    def export_csv(path)
      File.open(path, 'w') do |io|
        io.puts csv_row(CSV_HEADER)
        ErrorTree::ROOT.each_leaf do |leaf|
          io.puts csv_row([leaf.ancestors.first.id,
                           leaf.ancestors[1]&.id,
                           leaf.id,
                           leaf.path,
                           leaf.severity,
                           leaf.title,
                           @counts[leaf],
                           @printed[leaf]])
        end
      end
    rescue SystemCallError => e
      @out.puts "[#{TOOL}] could not write CSV export to #{path}: #{e.message}"
    end

    private

    CSV_HEADER = %w[category subcategory id path severity description detected reported].freeze

    def csv_row(fields)
      fields.map { |f|
        s = f.to_s
        s.match?(/[",\r\n]/) ? %("#{s.gsub('"', '""')}") : s
      }.join(',')
    end

    # Detections of a node and of everything below it.
    def subtree_count(node)
      count = @counts[node]
      node.children.each { |c| count += subtree_count(c) }
      count
    end

    def total_for(severity)
      @counts.sum { |node, n| node.severity == severity ? n : 0 }
    end

    def plural(count, word)
      count == 1 ? word : "#{word}s"
    end

    def visible_children(node)
      node.children.select { |c| subtree_count(c).positive? }
    end

    def print_subtree(node, rail)
      children = visible_children(node)
      children.each_with_index do |child, i|
        last = i == children.size - 1
        print_summary_line(child, rail + (last ? '└─ ' : '├─ '))
        print_subtree(child, rail + (last ? '   ' : '│  '))
      end
    end

    def print_summary_line(node, prefix)
      label = node.depth.zero? && !node.leaf? ? "#{node.title} (#{node.id})" : node.id.to_s
      label += '  [warning]' if node.leaf? && node.severity == :warning
      count = subtree_count(node)
      value = node.leaf? ? count.to_s : "(#{count})"
      leader = SUMMARY_WIDTH - prefix.length - label.length - 2
      leader = 1 if leader < 1
      @out.puts format('%s%s %s %8s', prefix, label, '.' * leader, value)
    end

    def capped?(node)
      return false if @printed[node] < MAX_REPORTS_PER_KIND

      if @capped.add?(node)
        @out.puts "[#{TOOL}] [#{node.severity}] [#{node.path}] reported #{MAX_REPORTS_PER_KIND} times; " \
                  'further occurrences are suppressed (see the end-of-trace summary)'
      end
      true
    end

    def emit(node, location, message)
      where = Array(location).map { |f| "[#{f}]" }.join(' ')
      @out.puts "[#{TOOL}] #{where} [#{node.severity}] [#{node.path}] #{message}"
    end
  end
end
