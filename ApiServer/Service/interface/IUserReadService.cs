
using Google.Protobuf;
using LoginApiServer.Model;

namespace LoginApiServer.Service.Interface
{
    public interface IUserReadService
    {
        Task<UserResponse> GetUserFromUserid(string id);
    }
}
