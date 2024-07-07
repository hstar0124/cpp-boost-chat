

namespace LoginApiServer.Service.Interface
{
    public interface IUserWriteService
    {
        Task<UserResponse> LoginUser(LoginRequest request);
        Task<UserResponse> CreateUser(CreateUserRequest request);
        Task<UserResponse> DeleteUser(DeleteUserRequest request);
        Task<UserResponse> UpdateUser(UpdateUserRequest request);
    }
}
