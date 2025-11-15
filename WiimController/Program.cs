using System.Device.Gpio;
using Microsoft.Extensions.Configuration;
using WiimController.Classes;
using WiimController.Services;


class Program
{
    static async Task Main()
    {
        var config = new ConfigurationBuilder()
            .AddJsonFile("appsettings.json", optional: true, reloadOnChange: true)
            .AddEnvironmentVariables()
            .Build();

        var baseUrl = config["ApiSettings:BaseUrl"];

        Console.WriteLine("Starting Button HTTP App...");
        bool isPi = Environment.OSVersion.Platform == PlatformID.Unix;
        Console.WriteLine(isPi ? "Running on Raspberry Pi (GPIO mode)" : "Running on Desktop (Keyboard mode)");

        var handler = new HttpClientHandler
        {
            ServerCertificateCustomValidationCallback = HttpClientHandler.DangerousAcceptAnyServerCertificateValidator
        };
        using var httpClient = new HttpClient(handler);

        WiimService _wiimService = new WiimService(httpClient, baseUrl);
        await _wiimService.SetPlayerStatus();

        if (isPi)
        {
            await RunGpioMode(_wiimService);
        }
        else
        {
            await RunKeyboardMode(_wiimService);
        }
    }

    static async Task RunKeyboardMode(WiimService wiimService)
    {
        Console.WriteLine("Press 1 or 2 to simulate button presses. Press Q to quit.");
        while (true)
        {
            if (Console.KeyAvailable)
            {
                var key = Console.ReadKey(true).Key;
                if (key == ConsoleKey.Q)
                {
                    Console.WriteLine("Exiting...");
                    break;
                }

                switch (key)
                {
                    case ConsoleKey.D1:
                        Console.WriteLine("Button 1 simulated.");

                        await wiimService.GetDeviceStatusAsync();
                        break;
                    case ConsoleKey.D2:
                        Console.WriteLine("Button 2 simulated.");
                        await wiimService.GetPlayerStatus();
                        break;
                    case ConsoleKey.D3:
                        Console.WriteLine("Button 3 simulated.");
                        //nothing
                        await wiimService.Resume();
                        break;
                    case ConsoleKey.D4:
                        Console.WriteLine("Button 4 simulated.");
                        await wiimService.Pause();
                        break;
                    case ConsoleKey.D5:
                        Console.WriteLine("Button 5 simulated.");
                        //nothing
                        var volume = 50;
                        await wiimService.SetVolume(volume);
                        break;
                    case ConsoleKey.D6:
                        Console.WriteLine("Button 6 simulated.");
                        await wiimService.PlayNextSong();
                        break;
                    case ConsoleKey.D8:
                        Console.WriteLine("Button 8 simulated.");
                        await wiimService.PlayPlaylist(1);
                        break;
                    default:
                        Console.WriteLine("7");
                        await wiimService.PlayPreviousSong();
                        break;
                }
            }

            await Task.Delay(50);
        }
    }

    static async Task RunGpioMode(WiimService wiimService)
    {
        using var controller = new GpioController();

        int btnNext3Pin = 26;
        int btnPrevious4Pin = 23;
        int btnPlaylist = 25;
       
        controller.OpenPin(btnNext3Pin, PinMode.InputPullUp);
        controller.OpenPin(btnPrevious4Pin, PinMode.InputPullUp);
        controller.OpenPin(btnPlaylist, PinMode.InputPullUp);

        //ToDo Rotary Knob for volume
        var encoder = new RotaryEncoder(
            wiimService: wiimService,
            pinA: 17,
            pinB: 22,
            pinButton: 27,
            controller: controller
        );

        while (true)
        {

            // Detect button press

            if (controller.Read(btnNext3Pin) == PinValue.Low)
            {
                Console.WriteLine("Button 3 pressed.");
                await wiimService.PlayNextSong();
                await Task.Delay(50);
            }

            if (controller.Read(btnPrevious4Pin) == PinValue.Low)
            {
                Console.WriteLine("Button 4 pressed.");
                await wiimService.PlayPreviousSong();
                await Task.Delay(50);
            }

            if (controller.Read(btnPlaylist) == PinValue.Low)
            {
                Console.WriteLine("Button playlist pressed.");
                await wiimService.PlayPlaylist(1);
                await Task.Delay(50);
            }

            await Task.Delay(50);
        }
    }
}