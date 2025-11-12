using System;
using System.Device.Gpio;
using System.Net.Http;
using System.Threading.Tasks;
using Microsoft.Extensions.Configuration;
using WiimController.Services;
using Microsoft.Extensions.Configuration.Json;


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
            await RunGpioMode(httpClient);
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
                        await wiimService.PlayNextSong();
                        break;
                    default:
                        Console.WriteLine("Invalid key. Use 1, 2, or Q to quit.");
                        break;
                }
            }

            await Task.Delay(50);
        }
    }

    static async Task RunGpioMode(HttpClient httpClient)
    {
        using var controller = new GpioController();

        int button1Pin = 17;
        int button2Pin = 27;

        controller.OpenPin(button1Pin, PinMode.InputPullUp);
        controller.OpenPin(button2Pin, PinMode.InputPullUp);

        Console.WriteLine("Listening for GPIO button presses (Ctrl+C to exit)...");

        while (true)
        {
            if (controller.Read(button1Pin) == PinValue.Low)
            {
                Console.WriteLine("Button 1 pressed.");
                await httpClient.GetAsync("https://example.com/api/button1");
                await Task.Delay(500); // debounce
            }

            if (controller.Read(button2Pin) == PinValue.Low)
            {
                Console.WriteLine("Button 2 pressed.");
                await httpClient.GetAsync("https://example.com/api/button2");
                await Task.Delay(500);
            }

            await Task.Delay(50);
        }
    }
}