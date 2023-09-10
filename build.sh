source ~/zephyrproject/zephyr/zephyr-env.sh
west build -p auto -b healthypi5_esp32c3 . -- -DBOARD_ROOT=.