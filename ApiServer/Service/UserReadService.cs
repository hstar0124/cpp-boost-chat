using ApiServer.Model.Entity;
using Google.Protobuf.WellKnownTypes;
using Grpc.Core;
using LoginApiServer.Model;
using LoginApiServer.Repository.Interface;
using LoginApiServer.Service.Interface;

namespace LoginApiServer.Service
{
    public class UserReadService : IUserReadService
    {
        private readonly ILogger<UserReadService> _logger;
        private readonly IUserRepository _userRepository;

        public UserReadService(ILogger<UserReadService> logger, IUserRepository userRepository)
        {
            _logger = logger;
            _userRepository = userRepository;
        }

        public UserResponse GetUserFromUserid(string id)
        {
            var status = UserStatusCode.Success;

            (status, UserEntity storedUser) = _userRepository.GetUserFromUserid(id);
            
            if (status != UserStatusCode.Success)
            {
                _logger.LogError("An error occurred while getting the User for UserId {UserId}.", id);
                return new UserResponse
                {
                    Status = UserStatusCode.Failure,
                    Message = "An error occurred while getting the User"
                };
            }

            // DB에서 가져온 값을 그대로 유저에게 넘겨주지 않도록 주의
            var userResponse = new GetUserResponse
            {
                UserId = storedUser.UserId,
                Username = storedUser.Username,
                Email = storedUser.Email
            };

            return new UserResponse
            {
                Status = status,
                Message = "User retrieved successfully",
                Content = Any.Pack(userResponse)
            };
            
        }

    }
}
