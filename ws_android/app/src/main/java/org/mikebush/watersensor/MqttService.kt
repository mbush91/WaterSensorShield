package org.mikebush.watersensor

import android.app.Service
import android.content.Intent
import android.os.IBinder
import org.eclipse.paho.android.service.MqttAndroidClient
import org.eclipse.paho.client.mqttv3.IMqttToken
import org.eclipse.paho.client.mqttv3.MqttCallback
import org.eclipse.paho.client.mqttv3.MqttConnectOptions
import org.eclipse.paho.client.mqttv3.MqttMessage

class MqttService : Service() {

    private lateinit var mqttAndroidClient: MqttAndroidClient

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val serverUri = "tcp://broker.hivemq.com:1883"  // Example broker; replace with your own

        mqttAndroidClient = MqttAndroidClient(applicationContext, serverUri, "AndroidClient")
        val options = MqttConnectOptions()

        try {
            mqttAndroidClient.connect(options, null, object : IMqttToken {
                override fun getClient() = mqttAndroidClient
                override fun getActionCallback() = this
                override fun getTopics() = arrayOf("your/topic")
                override fun getUserContext() = null
                override fun isComplete() = true
                override fun getException() = null
                override fun getSessionPresent() = false
                override fun getGrantedQos() = intArrayOf(0)
                override fun getMessageId() = 0
                override fun getResponse() = null
                override fun waitForCompletion() {}
                override fun waitForCompletion(timeout: Long) {}

                override fun onSuccess(asyncActionToken: IMqttToken?) {
                    mqttAndroidClient.subscribe("your/topic", 0)  // Replace "your/topic" with your topic
                }

                override fun onFailure(asyncActionToken: IMqttToken?, exception: Throwable?) {
                    // Handle connection failure
                }
            })
        } catch (e: Exception) {
            e.printStackTrace()
        }

        mqttAndroidClient.setCallback(object : MqttCallback {
            override fun connectionLost(cause: Throwable?) {
                // Handle connection lost
            }

            override fun messageArrived(topic: String?, message: MqttMessage?) {
                // Handle message arrived
                // Example: handle new message with message.toString()
            }

            override fun deliveryComplete(token: IMqttToken?) {
                // Handle delivery complete
            }
        })

        return START_STICKY
    }

    override fun onBind(intent: Intent?): IBinder? {
        return null
    }
}
