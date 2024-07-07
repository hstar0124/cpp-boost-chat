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

        public async Task<UserResponse> GetUserFromUserid(string id)
        {
            var status = UserStatusCode.Success;

            (status, GetUserResponse response) = await _userRepository.GetUserFromUserid(id);
            
            if (status != UserStatusCode.Success)
            {
                _logger.LogError("An error occurred while getting the User for UserId {UserId}.", id);
                return new UserResponse
                {
                    Status = UserStatusCode.Failure,
                    Message = "An error occurred while getting the User"
                };
            }

            return new UserResponse
            {
                Status = status,
                Message = "User retrieved successfully",
                Content = Any.Pack(response)
            };
            
        }

    }
}
