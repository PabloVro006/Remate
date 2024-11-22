install i2c-tools

copy the display_ip.service in
/etc/systemd/system/server_daemon.service

sudo systemctl daemon-reload
sudo systemctl enable server_daemon.service
sudo systemctl start server_daemon.service
