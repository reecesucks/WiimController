using System.Device.Gpio;
using Microsoft.Extensions.Configuration;
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

        int btnResume1Pin = 17;
        int btnPause2Pin = 27;
        int btnNext3Pin = 37;
        int btnPrevious4Pin = 47;

        //ToDo Rotary Knob for volume

        controller.OpenPin(btnResume1Pin, PinMode.InputPullUp);
        controller.OpenPin(btnPause2Pin, PinMode.InputPullUp);
        controller.OpenPin(btnNext3Pin, PinMode.InputPullUp);
        controller.OpenPin(btnPrevious4Pin, PinMode.InputPullUp);

        Console.WriteLine("Listening for GPIO button presses (Ctrl+C to exit)...");

        while (true)
        {
            if (controller.Read(btnResume1Pin) == PinValue.Low)
            {
                Console.WriteLine("Button 1 pressed.");
                await wiimService.Resume();
                await Task.Delay(500); // debounce
            }

            if (controller.Read(btnPause2Pin) == PinValue.Low)
            {
                Console.WriteLine("Button 2 pressed.");
                await wiimService.Pause();
                await Task.Delay(500);
            }

            if (controller.Read(btnNext3Pin) == PinValue.Low)
            {
                Console.WriteLine("Button 1 pressed.");
                await wiimService.PlayNextSong();
                await Task.Delay(500); // debounce
            }

            if (controller.Read(btnPrevious4Pin) == PinValue.Low)
            {
                Console.WriteLine("Button 2 pressed.");
                await wiimService.PlayPreviousSong();
                await Task.Delay(500);
            }

            await Task.Delay(50);
        }
    }
}