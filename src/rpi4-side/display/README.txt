install i2c-tools

copy the display_ip.service in
/etc/systemd/system/display_ip.service

sudo systemctl daemon-reload
sudo systemctl enable display_ip.service
sudo systemctl start display_ip.service
