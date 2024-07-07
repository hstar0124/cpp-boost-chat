using System.ComponentModel.DataAnnotations.Schema;
using System.ComponentModel.DataAnnotations;

namespace ApiServer.Model
{
    public class UserDto
    {
        public long Id { get; set; }
        public string UserId { get; set; }
        public string Password { get; set; }
        public string Username { get; set; }
        public string Email { get; set; }
        public string IsAlive { get; set; }
    }
}
