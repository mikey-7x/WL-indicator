import requests
from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.label import Label
from kivy.uix.togglebutton import ToggleButton
from kivy.clock import Clock
from kivy.core.window import Window

# ⚠️ CHANGE THIS to the IP address from your Arduino Serial Monitor
ESP32_IP = "http://10.254.217.187"

class WaterLevelApp(App):
    def build(self):
        Window.size = (360, 640) # Simulate a mobile screen size
        
        self.layout = BoxLayout(orientation='vertical', padding=20, spacing=20)
        
        # Title
        self.title = Label(text="Water Level Monitor", font_size=30, size_hint=(1, 0.2))
        self.layout.add_widget(self.title)

        # Gauge Label
        self.gauge_label = Label(text="Water Level: -- %", font_size=40, bold=True, size_hint=(1, 0.4))
        self.layout.add_widget(self.gauge_label)

        # Motor Switch
        self.motor_btn = ToggleButton(text="Motor: OFF", font_size=24, size_hint=(1, 0.2), background_color=(0.8, 0.2, 0.2, 1))
        self.motor_btn.bind(on_press=self.toggle_motor)
        self.layout.add_widget(self.motor_btn)
        
        # Connection Status
        self.status_label = Label(text="Connecting...", font_size=16, size_hint=(1, 0.2), color=(0.7, 0.7, 0.7, 1))
        self.layout.add_widget(self.status_label)

        # Schedule the app to ask the ESP32 for data every 1 second
        Clock.schedule_interval(self.fetch_data, 1.0)

        return self.layout

    def toggle_motor(self, instance):
        try:
            if instance.state == 'down':
                requests.get(f"{ESP32_IP}/motor/on", timeout=2)
                instance.text = "Motor: ON"
                instance.background_color = (0.2, 0.8, 0.2, 1) # Green
            else:
                requests.get(f"{ESP32_IP}/motor/off", timeout=2)
                instance.text = "Motor: OFF"
                instance.background_color = (0.8, 0.2, 0.2, 1) # Red
        except requests.exceptions.RequestException:
            self.status_label.text = "Error: Could not send command."

    def fetch_data(self, dt):
        try:
            response = requests.get(f"{ESP32_IP}/status", timeout=2)
            data = response.json()
            
            level = data.get("waterLevel", 0)
            self.gauge_label.text = f"Water Level: {level} %"
            self.status_label.text = "Connected via Wi-Fi"
            self.status_label.color = (0, 1, 0, 1)

            # Sync button state if it was changed physically (fail-safe)
            motor_state = data.get("motor", False)
            if motor_state and self.motor_btn.state == 'normal':
                self.motor_btn.state = 'down'
                self.motor_btn.text = "Motor: ON"
                self.motor_btn.background_color = (0.2, 0.8, 0.2, 1)
            elif not motor_state and self.motor_btn.state == 'down':
                self.motor_btn.state = 'normal'
                self.motor_btn.text = "Motor: OFF"
                self.motor_btn.background_color = (0.8, 0.2, 0.2, 1)

        except requests.exceptions.RequestException:
            self.gauge_label.text = "Water Level: -- %"
            self.status_label.text = "Disconnected. Checking ESP32..."
            self.status_label.color = (1, 0, 0, 1)

if __name__ == '__main__':
    WaterLevelApp().run()
