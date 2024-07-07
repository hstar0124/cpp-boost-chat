
using ApiServer.Model;
using ApiServer.Model.Entity;
using LoginApiServer.Model;

namespace LoginApiServer.Repository.Interface
{
    public interface IUserRepository
    {
        Task<UserStatusCode> CreateUser(UserDto userDto);
        Task<UserStatusCode> UpdateUser(UserDto userDto);
        Task<UserStatusCode> DeleteUser(UserDto userDto);
        Task<(UserStatusCode, GetUserResponse)> GetUserFromUserid(string id);
        Task<(UserStatusCode, UserDto)> ValidateUserCredentials(UserDto userDto);
    }
}
