source ~/zephyrproject/zephyr/zephyr-env.sh
west build -t clean --pristine -b healthypi5_esp32c3 . -- -DBOARD_ROOT=.