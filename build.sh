source ~/zephyrproject/zephyr/zephyr-env.sh
#west build -p auto -b healthypi5_esp32c3 . -- -DBOARD_ROOT=.
west build -p auto -b healthypi5_esp32c3 --sysbuild . -- -DBOARD_ROOT=/Users/akw/Documents/GitHub/healthypi5_esp32c3_zephyr
