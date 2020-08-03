#!/usr/bin/env bash

trace() {
    quiet="--quiet"
    lttng-sessiond --daemonize $quiet
    lttng $quiet create clprof                                      
    #Using blocking more to trace event record loss
    lttng $quiet enable-channel --userspace --blocking-timeout=inf blocking-channel     

    lttng $quiet enable-event --channel=blocking-channel --userspace lttng_ust_opencl:*  
    lttng $quiet enable-event --channel=blocking-channel --userspace lttng_ust_opencl_build:infos*
    lttng $quiet enable-event --channel=blocking-channel --userspace lttng_ust_opencl_profiling:*
    lttng $quiet enable-event --channel=blocking-channel --userspace lttng_ust_opencl_arguments:kernel_info
    lttng $quiet enable-event --channel=blocking-channel --userspace lttng_ust_opencl_devices:device_name

    lttng $quiet add-context --userspace --channel=blocking-channel -t vpid -t vtid

    #Preventing trace event record loss
    export LTTNG_UST_ALLOW_BLOCKING=1

    lttng $quiet start
    LD_PRELOAD=/home/videau/opt/tracer/lib/tracer_opencl.so "$@"
    lttng $quiet stop
    lttng $quiet destroy
}

summary() {
    if [ -z $path ]
    then
        lltng_last_session=$(ls -dt $HOME/lttng-traces/* | head -1)
        echo "Trace location " $lltng_last_session
    else
        lltng_last_session=$1
    fi
    babeltrace2 --plugin-path=$(dirname "$0") --component=sink.iprof.dispatch  --params="display=$display" ${lltng_last_session}
}

#  _
# |_) _. ._ _ o ._   _     /\  ._ _
# |  (_| | _> | | | (_|   /--\ | (_| \/
#                    _|           _|
display="compact"
replay=false
while (( "$#" )); do
    case "$1" in
        -e|--extented)
            display="extended"; shift;
            ;;
        -r|--replay)
            shift; path=$@; replay=true;
            ;;
        *)
            break 
            ;;
    esac
done

if [ $replay == false ]; then
    trace "$@"
fi 
    
summary $path

