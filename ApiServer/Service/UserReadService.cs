using ApiServer.CustomException;
using Google.Protobuf.WellKnownTypes;
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
            var (status, response) = await _userRepository.GetUserFromUserid(id);

            if (status != UserStatusCode.Success)
            {
                _logger.LogError("An error occurred while getting the User for UserId {UserId}.", id);
                throw new UserNotFoundException("An error occurred while getting the User");
            }

            return new UserResponse
            {
                Status = UserStatusCode.Success,
                Message = "User retrieved successfully",
                Content = Any.Pack(response)
            };
        }

    }
}
