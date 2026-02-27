#  Header Location

github.com/argonne-lcf/level-zero-spec/tree/ddi_ver

### Note
Different from https://github.com/oneapi-src/level-zero/tree/v1.28.2/include 

# Step:

- Sync the fork with reach
- Sync the bran ddi_ver

```
git clone -b ddi_ver https://github.com/argonne-lcf/level-zero-spec.git
cd level-zero-spec/scripts/
uv pip install -r third_party/requirements.txt
cd script
```
Get version number
```
version=$(grep -oP "version \d+\.\d+" core/driver.yml | tail -1 | awk '{print $2}')
echo $version
```

use It to generate the spec and verify that everything match
```
python run.py  --ver $version --\!debug --\!html
grep "define ZE_API_VERSION_CURRENT_M" ../include/ze_api.h
```
- Now header have been generated in `level-zero-spec/include`. Copy that in `$THAPI_ROOT/backend/ze/include`
(carefull for removed file)

- This does contain the layers/loader and zex. 

  - We got those from `https://github.com/oneapi-src/level-zero/tree/v1.28.2/include`

- Try to compile

## Problem

```
tracer_ze.c:197:8: error: use of undeclared identifier 'ZE_STRUCTURE_TYPE_DEVICE_CACHE_LINE_SIZE_EXT'; did you mean 'ZE_STRUCTURE_TYPE_DEVICE_CACHELINE_SIZE_EXT'?
  197 |   case ZE_STRUCTURE_TYPE_DEVICE_CACHE_LINE_SIZE_EXT:
      |        ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      |        ZE_STRUCTURE_TYPE_DEVICE_CACHELINE_SIZE_EXT
```

- Due to intel lack of naming consistancy, maybe need to update `struct_type_conversion_table` n in `ze_model.rb`

```
tracer_ze.c:8456:20: error: unused function 'zelTracerResetAllCallbacks_hid' [-Werror,-Wunused-function]
 8456 | static ze_result_t zelTracerResetAllCallbacks_hid(zel_tracer_handle_t hTracer)__attribute__ ((alias ("zelTracerResetAllCallbacks")));
      |                    ^~~~~~~~~~~~~~~~~
```
Need to modify the `$zel_commands.each` who generate `#{c.decl_hidden_alias};` in `gen_ze.rb`

Run time error: When running babeltrace_thapi (`iprof -t`)
```
ogrammableParamValueInfoExp>': uninitialized constant ZE::ZETMetricProgrammableParamValueInfoExp::ZET_MAX_METRIC_PROGRAMMABLE_VALUE_DESCRIPTION_EXP (NameError)

           :description, [ :char, ZET_MAX_METRIC_PROGRAMMABLE_VALUE_DESCRIPTION_EXP ]
```
Update in `backends/ze/gen_ze_library.rb` Moduel ZE. 
TODO: h2yaml should be able to find all the constant defined
