install i2c-tools

copy the display.service in
/etc/systemd/system/display.service

sudo systemctl daemon-reload
sudo systemctl enable display.service
sudo systemctl start display_ip.service
