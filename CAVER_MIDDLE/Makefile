.PHONY: switch switch-p4 switch-ctrl clean


switch: switch-p4 switch-ctrl

switch-p4: caver_middle_new.p4
	sudo -E $(SDE)/p4_build.sh $<

switch-ctrl: test_control/ctrl.c test_control/headers.h test_control/switch_config.h config.h
	gcc -I$$SDE_INSTALL/include -g -O2 -std=gnu11 -L/usr/local/lib -L$$SDE_INSTALL/lib \
		$< -o test \
		-ldriver -lbfsys -lbfutils -lbf_switchd_lib -lm -lpthread

# 添加 debug 目标，使用 gdb 调试
debug: switch-ctrl
	gdb ./test



clean:
	-rm -f contrl bf_drivers.log* zlog-cfg-cur

# .DEFAULT_GOAL :=
