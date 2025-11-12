using System.Text.Json;
using WiimController.Classes;
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

        private WiimApiResult GetFailedResult(string endpoint, Exception ex) 
        {
            //ToDo: add logging for failed request
            return new WiimApiResult
            {
                Success = false,
                Message = ex.Message
            };
        }

        private WiimApiResult GetRequestResult(bool isSuccessStatusCode, string jsonResponse)
        {
            //ToDo: add logging for failed request
            return new WiimApiResult
            {
                Success = isSuccessStatusCode && jsonResponse.Trim().Equals("OK", StringComparison.OrdinalIgnoreCase),
                Message = jsonResponse
            };
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

        public async Task<PlayerStatus> GetPlayerStatus()
        {
            try
            {
                var response = await _httpClient.GetAsync("httpapi.asp?command=getPlayerStatus");
                string json = await response.Content.ReadAsStringAsync();

                var playerStatus = JsonSerializer.Deserialize<PlayerStatus>(
                                        json,
                                        new JsonSerializerOptions { PropertyNameCaseInsensitive = true });

                return playerStatus;
            }
            catch (Exception ex)
            {
                return null;
            }
        }

        public async Task<WiimApiResult> PlayNextSong()
        {
            try
            {
                var response = await _httpClient.GetAsync("httpapi.asp?command=setPlayerCmd:next");
                string text = await response.Content.ReadAsStringAsync();

                return GetRequestResult(response.IsSuccessStatusCode, text);
            }
            catch (Exception ex)
            {
                return GetFailedResult(nameof(PlayNextSong), ex);
            }
        }

        public async Task<WiimApiResult> PlayPreviousSong()
        {
            try
            {
                var response = await _httpClient.GetAsync("httpapi.asp?command=setPlayerCmd:prev");
                string text = await response.Content.ReadAsStringAsync();

                return GetRequestResult(response.IsSuccessStatusCode, text);
            }
            catch (Exception ex)
            {
                return GetFailedResult(nameof(PlayPreviousSong), ex);
            }
        }

        public async Task<WiimApiResult> Pause()
        {
            try
            {
                var response = await _httpClient.GetAsync("httpapi.asp?command=setPlayerCmd:pause");
                string jsonResponse = await response.Content.ReadAsStringAsync();

                return GetRequestResult(response.IsSuccessStatusCode, jsonResponse);
            }
            catch (Exception ex)
            {
                return GetFailedResult(nameof(Pause), ex);
            }
        }

        public async Task<WiimApiResult> Resume()
        {
            try
            {
                var response = await _httpClient.GetAsync("httpapi.asp?command=setPlayerCmd:resume");
                string jsonResponse = await response.Content.ReadAsStringAsync();

                return GetRequestResult(response.IsSuccessStatusCode, jsonResponse);
            }
            catch (Exception ex)
            {
                return GetFailedResult(nameof(Resume), ex);
            }
        }

        public async Task<WiimApiResult> SetVolume(int value)
        {
            try
            {
                var response = await _httpClient.GetAsync($"httpapi.asp?command=setPlayerCmd:vol:{value}");
                string jsonResponse = await response.Content.ReadAsStringAsync();

                return GetRequestResult(response.IsSuccessStatusCode, jsonResponse);

            }
            catch (Exception ex)
            {
                return GetFailedResult(nameof(SetVolume), ex);
            }
        }
    }
}
