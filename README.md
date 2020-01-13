Aim : to send data using tls certificate to http://test.mosquitto.org/ using nodemcu and 
MQTT pubsub

- refer http://test.mosquitto.org/
- download the mosquitto.org.crt from above site
- convet the certificate from .crt to .der form using 
openssl x509 -in cert.crt -outform der -out cert.der

- create folder of with name data
- copy .der certificate in the folder

-get the fingerprint using following command which will required in the code
openssl x509 -in  mqttserver.crt -sha1 -noout -fingerprint

- use sketch data upload tool to upload the certificate.der file in the spiffs

- give the file name and fingerprint key in the code.

- upload code in nodemcu and verify with the mosquitto_sub command