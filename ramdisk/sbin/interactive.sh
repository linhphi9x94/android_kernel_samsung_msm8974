#!/res/busybox sh

export PATH=/res/asset:$PATH

echo "interactive" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo "interactive" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
echo "interactive" > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
echo "interactive" > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor

echo "340"	> /sys/devices/system/cpu/cpufreq/interactive/multi_enter_load
echo "99000"	> /sys/devices/system/cpu/cpufreq/interactive/multi_enter_time
echo "90"	> /sys/devices/system/cpu/cpufreq/interactive/multi_exit_load
echo "299000"	> /sys/devices/system/cpu/cpufreq/interactive/multi_exit_time
echo "90"	> /sys/devices/system/cpu/cpufreq/interactive/single_enter_load
echo "199000"	> /sys/devices/system/cpu/cpufreq/interactive/single_enter_time
echo "60"	> /sys/devices/system/cpu/cpufreq/interactive/single_exit_load
echo "99000"	> /sys/devices/system/cpu/cpufreq/interactive/single_exit_time
echo "0"	> /sys/devices/system/cpu/cpufreq/interactive/param_index
echo "20000"	> /sys/devices/system/cpu/cpufreq/interactive/timer_rate
echo "20000"	> /sys/devices/system/cpu/cpufreq/interactive/timer_slack
echo "40000"	> /sys/devices/system/cpu/cpufreq/interactive/min_sample_time
echo "1190400"	> /sys/devices/system/cpu/cpufreq/interactive/hispeed_freq
echo "99"	> /sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load
echo "20000 1400000:80000 1500000:40000 1700000:20000"	> /sys/devices/system/cpu/cpufreq/interactive/above_hispeed_delay
echo "85 1400000:90 1700000:95"	> /sys/devices/system/cpu/cpufreq/interactive/target_loads
echo "100000"	> /sys/devices/system/cpu/cpufreq/interactive/sampling_down_factor
echo "1"	> /sys/devices/system/cpu/cpufreq/interactive/param_index
echo "19000"	> /sys/devices/system/cpu/cpufreq/interactive/above_hispeed_delay
echo "95"	> /sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load
echo "1497600"	> /sys/devices/system/cpu/cpufreq/interactive/hispeed_freq
echo "59000"	> /sys/devices/system/cpu/cpufreq/interactive/min_sample_time
echo "60 800000:65 1400000:65 1700000:75"	> /sys/devices/system/cpu/cpufreq/interactive/target_loads
echo "20000"	> /sys/devices/system/cpu/cpufreq/interactive/timer_rate
echo "150000"	> /sys/devices/system/cpu/cpufreq/interactive/sampling_down_factor
echo "2"	> /sys/devices/system/cpu/cpufreq/interactive/param_index
echo "19000"	> /sys/devices/system/cpu/cpufreq/interactive/above_hispeed_delay
echo "90"	> /sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load
echo "1728000"	> /sys/devices/system/cpu/cpufreq/interactive/hispeed_freq
echo "79000"	> /sys/devices/system/cpu/cpufreq/interactive/min_sample_time
echo "50 800000:60 1400000:65"	> /sys/devices/system/cpu/cpufreq/interactive/target_loads
echo "20000"	> /sys/devices/system/cpu/cpufreq/interactive/timer_rate
echo "200000"	> /sys/devices/system/cpu/cpufreq/interactive/sampling_down_factor
echo "3"	> /sys/devices/system/cpu/cpufreq/interactive/param_index
echo "19000"	> /sys/devices/system/cpu/cpufreq/interactive/above_hispeed_delay
echo "85"	> /sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load
echo "1958400"	> /sys/devices/system/cpu/cpufreq/interactive/hispeed_freq
echo "99000"	> /sys/devices/system/cpu/cpufreq/interactive/min_sample_time
echo "50 1400000:60"	> /sys/devices/system/cpu/cpufreq/interactive/target_loads
echo "20000"	> /sys/devices/system/cpu/cpufreq/interactive/timer_rate
echo "300000"	> /sys/devices/system/cpu/cpufreq/interactive/sampling_down_factor
echo "0"	> /sys/devices/system/cpu/cpufreq/interactive/param_index
