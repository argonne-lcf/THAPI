require 'rbtree'

module ZEModel

  # A collection of pairwise disjoint intervals, keyed on their base pointer.
  class AllocationMap

    class OverlapError < StandardError
      attr_reader :entries
      def initialize(entries)
        @entries = entries
        super('range overlaps a live entry')
      end
    end

    def initialize
      @tree = RBTree.new
    end

    def [](addr)
      pair = @tree.upper_bound(addr)
      pair && addr < pair[1].base + pair[1].size ? pair[1] : nil
    end

    def insert(obj)
      clashes = overlapping(obj.base, obj.size)
      raise OverlapError, clashes unless clashes.empty?
      @tree[obj.base] = obj
    end

    def delete(base)
      @tree.delete(base)
    end

    # The entries meeting [base, base + size), in ascending base order.
    def overlapping(base, size)
      return [] unless size.positive?
      hits = @tree.bound(base, base + size - 1).map { |_base, obj| obj }
      pred = @tree.upper_bound(base - 1)
      hits.unshift(pred[1]) if pred && pred[1].base + pred[1].size > base
      hits
    end

    def each(&block)
      @tree.each_value(&block)
    end
    alias each_value each

    def empty?
      @tree.empty?
    end

    def size
      @tree.size
    end
  end
end
