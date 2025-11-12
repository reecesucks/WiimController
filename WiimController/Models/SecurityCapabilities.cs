using System.Text.Json.Serialization;

namespace WiimController.Models
{
    public class SecurityCapabilities
    {
        [JsonPropertyName("ver")]
        public string Ver { get; set; }

        [JsonPropertyName("aes_ver")]
        public string AesVer { get; set; }
    }
}
