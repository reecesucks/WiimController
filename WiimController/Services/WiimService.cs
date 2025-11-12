using System.Text;
using System.Text.Json;
using WiimController.Models;

namespace WiimController.Services
{
    public class WiimService
    {
        private readonly HttpClient _httpClient;

        public WiimService(HttpClient httpClient, string baseUrl)
        {
            var handler = new HttpClientHandler
            {
                ServerCertificateCustomValidationCallback = HttpClientHandler.DangerousAcceptAnyServerCertificateValidator
            };
            _httpClient = httpClient;
            _httpClient.DefaultRequestHeaders.UserAgent.ParseAdd("WiimController/1.0");

            _httpClient.BaseAddress = new Uri(baseUrl);           
        }

        public async Task<DeviceStatus> GetDeviceStatusAsync()
        {
            try
            {
                var response = await _httpClient.GetAsync("httpapi.asp?command=getStatusEx");
                string json = await response.Content.ReadAsStringAsync();

                var deviceStatus = JsonSerializer.Deserialize<DeviceStatus>(json, new JsonSerializerOptions
                {
                    PropertyNameCaseInsensitive = true
                });

                return deviceStatus;

            }
            catch (Exception ex)
            {
                //
            }

            return null;
        }

        public async Task<String> PlayNextSong()
        {
            try
            {
                var response = await _httpClient.GetAsync("httpapi.asp?command=setPlayerCmd:next");
                string json = await response.Content.ReadAsStringAsync();


                return json;

            }
            catch (Exception ex)
            {
                //
            }

            return null;
        }
    }
}
