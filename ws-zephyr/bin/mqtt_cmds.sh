cafile="./src/lib/pss_mqtt/cert/AmazonRootCA1.pem"
crt="./src/lib/pss_mqtt/cert/d1518bf7e3eb8e70a0c246dfaa78beaf44d34c718056e9dca3c8b70d38c5bdf3-certificate.pem.crt"
key="./src/lib/pss_mqtt/cert/d1518bf7e3eb8e70a0c246dfaa78beaf44d34c718056e9dca3c8b70d38c5bdf3-private.pem"
host="a2p0gdf34vn364-ats.iot.us-east-2.amazonaws.com"

msg=OFF

if [ "$1" == "ON" ]; then
    msg="ON"
fi

t1="homeassistant/sump/sensor"
t2="homeassistant/sump/pump"
t3="homeassistant/sump/batt"
t4="homeassistant/sump/availability"

mosquitto_pub --cafile $cafile --cert $crt --key $key -h $host -p 8883  -t $t1 -m "$msg"
mosquitto_pub --cafile $cafile --cert $crt --key $key -h $host -p 8883  -t $t2 -m "$msg"
mosquitto_pub --cafile $cafile --cert $crt --key $key -h $host -p 8883  -t $t3 -m "14.00"
mosquitto_pub --cafile $cafile --cert $crt --key $key -h $host -p 8883  -t $t4 -m "online"
