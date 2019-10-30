require 'minitest/autorun'
require 'babeltrace'
require 'babeltrace/ctf'

`rm -rf ./traces 2>&1` if Dir.exist? "./traces"
Dir.mkdir "./traces"
`lttng-sessiond --daemonize 2>&1`
`lttng stop 2>&1`
`lttng destroy --all 2>&1`

ENV["LIBOPENCL_SO"] = "../tracer.so"
require 'opencl_ruby_ffi'

at_exit do
  `lttng stop 2>&1`
  `lttng destroy --all 2>&1`
end

$counter = 0

class TracerTest < Minitest::Test

  def create_session(profiling: false, source: false, dump: nil)
    $counter += 1
    `lttng create my-userspace-test-opencl-session --output=./traces/#{$counter} 2>&1`
    `lttng enable-channel --userspace --blocking-timeout=inf blocking-channel 2>&1`
    `lttng enable-event --channel=blocking-channel --userspace lttng_ust_opencl:* 2>&1`
    `lttng enable-event --channel=blocking-channel --userspace lttng_ust_opencl_profiling:* 2>&1` if profiling
    `lttng enable-event --channel=blocking-channel --userspace lttng_ust_opencl_source:* 2>&1` if source
    `lttng start 2>&1`
    "/traces/#{$counter}"
  end

  def delete_session
    `lttng stop 2>&1`
    `lttng destroy --all 2>&1`
  end

  def test_basic_info_retrieval
    create_session
    plats = OpenCL::platforms
    names = plats.collect(&:name)
    delete_session
    c = Babeltrace::Context::new
    t = c.add_trace(path: "./traces/#{$counter}/ust/uid/1000/64-bit/")
    it = t.iter_create

    assert_equal(4 + 2 * 2 * plats.size, it.each.count)

    e = it.each
    ev = e.next
    assert_equal("lttng_ust_opencl:clGetPlatformIDs_start", ev.name)
    ev = e.next
    assert_equal("lttng_ust_opencl:clGetPlatformIDs_stop", ev.name)
    assert_equal(plats.length, ev.top_level_scope(:EVENT_FIELDS).value["num_platforms_val"])
    ev = e.next
    assert_equal("lttng_ust_opencl:clGetPlatformIDs_start", ev.name)
    assert_equal(plats.length, ev.top_level_scope(:EVENT_FIELDS).value["num_entries"])
    ev = e.next
    assert_equal("lttng_ust_opencl:clGetPlatformIDs_stop", ev.name)
    assert_equal(plats.collect(&:pointer).collect(&:to_i), ev.top_level_scope(:EVENT_FIELDS).value["platforms_vals"])
    names.each_with_index { |n|
      e.next
      ev = e.next
      assert_equal(n.size + 1, ev.top_level_scope(:EVENT_FIELDS).value["param_value_size_ret_val"])
      ev = e.next
      assert_equal(OpenCL::Platform::NAME, ev.top_level_scope(:EVENT_FIELDS).value["param_name"])
      assert_equal(n.size + 1, ev.top_level_scope(:EVENT_FIELDS).value["param_value_size"])
      ev = e.next
      assert_equal(n, ev.top_level_scope(:EVENT_FIELDS).value["param_value_vals"].unpack("A*").first)
    }

  end

end

