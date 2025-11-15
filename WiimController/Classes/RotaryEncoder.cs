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
        public int Count { get; private set; }

        public event Action<int>? Rotated;       // +1 / -1
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
        }

        private void OnEncoderARising(object sender, PinValueChangedEventArgs args)
        {
            int a = _controller.Read(_pinA) == PinValue.High ? 1 : 0;
            int b = _controller.Read(_pinB) == PinValue.High ? 1 : 0;

            int direction = (a == b) ? +1 : -1;

            Count += direction;
            Rotate(Count);
         //   Rotated?.Invoke(direction);
        }

        private async void Rotate(int direction)
        {
            _wiimService.Volume += direction;
            await _wiimService.SetVolume(_wiimService.Volume);
        }

        public async void CheckButton()
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
