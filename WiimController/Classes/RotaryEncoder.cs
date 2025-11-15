using System;
using System.Device.Gpio;
using WiimController.Services;

namespace WiimController.Classes
{
    public class RotaryEncoder : IDisposable
    {
        private readonly int _pinA;
        private readonly int _pinB;
        private readonly int _pinButton;
        private readonly WiimService _wiimService;
        private readonly GpioController _controller;
        private readonly Timer _rotationTimer;
        private const int RotationDelayMs = 25;
        private int _rotationbuffer;
        private int _count = 0;
        public event Action<int>? RotatedAction;       // +1 / -1
        public event Action? ButtonPressed;

        public RotaryEncoder(int pinA, int pinB, int pinButton, WiimService wiimService, GpioController? controller = null)
        {
            _pinA = pinA;
            _pinB = pinB;
            _pinButton = pinButton;

            _wiimService = wiimService;
            _controller = controller ?? new GpioController();

            // Init pins
            _controller.OpenPin(_pinA, PinMode.InputPullUp);
            _controller.OpenPin(_pinB, PinMode.InputPullUp);
            _controller.OpenPin(_pinButton, PinMode.InputPullUp);

            // Encoder interrupt on rising edge
            _controller.RegisterCallbackForPinValueChangedEvent(
                _pinA,
                PinEventTypes.Rising,
                OnEncoderARising
            );

            _controller.RegisterCallbackForPinValueChangedEvent(
                _pinButton,
                PinEventTypes.Rising,
                CheckButton
            );

            _rotationTimer = new Timer(OnRotationTimerElaped, null, Timeout.Infinite, Timeout.Infinite);
        }

        private void OnEncoderARising(object sender, PinValueChangedEventArgs args)
        {
            int a = _controller.Read(_pinA) == PinValue.High ? 1 : 0;
            int b = _controller.Read(_pinB) == PinValue.High ? 1 : 0;

            int direction = (a == b) ? +1 : -1;

            _rotationbuffer +=direction;
            _count += direction;

            _rotationTimer.Change(RotationDelayMs, Timeout.Infinite);

        }

        private void OnRotationTimerElaped(object? state)
        {
            if (_rotationbuffer != 0)
            {
                Rotated(_rotationbuffer);
                _rotationbuffer = 0;
            }
        }

        private async void Rotated(int rotationValue)
        {
            _wiimService.Volume += rotationValue;
            await _wiimService.SetVolume(_wiimService.Volume);
        }

        public async void CheckButton(object sender, PinValueChangedEventArgs args)
        {
            if (_controller.Read(_pinButton) == PinValue.Low)
            {
                await _wiimService.OnePause();
                //ButtonPressed?.Invoke();
            }
        }

        public void Dispose()
        {
            _controller?.Dispose();
        }
    }
}
