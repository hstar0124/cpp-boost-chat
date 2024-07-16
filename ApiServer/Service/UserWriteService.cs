using ApiServer.CustomException;
using ApiServer.Model;
using ApiServer.Model.Entity;
using Google.Protobuf.WellKnownTypes;
using LoginApiServer.Model;
using LoginApiServer.Repository.Interface;
using LoginApiServer.Service.Interface;
using LoginApiServer.Utils;
using System.Transactions;

namespace LoginApiServer.Service
{
    public class UserWriteService : IUserWriteService
    {
        private readonly ILogger<UserWriteService> _logger;
        private readonly IUserRepository _userRepository;
        private readonly ICacheRepository _cacheRepository;

        public UserWriteService (ILogger<UserWriteService> logger, IUserRepository userRepository, ICacheRepository cacheRepository)
        {
            _logger = logger;
            _userRepository = userRepository;
            _cacheRepository = cacheRepository;
        }

        public async Task<UserResponse> LoginUser(LoginRequest request)
        {
            var user = new UserDto
            {
                UserId = request.UserId,
                Password = request.Password
            };

            (var status, var userInfo) = await _userRepository.ValidateUserCredentials(user);

            if (status != UserStatusCode.Success)
            {
                throw new UserValidationException("User validation failed");
            }

            // 세션ID 생성
            string sessionId = Guid.NewGuid().ToString().Replace("-", "");

            // Redis에 세션 ID 저장
            status = await _cacheRepository.CreateSession(sessionId, userInfo.Id);
            if (status != UserStatusCode.Success)
            {
                _logger.LogError("An error occurred while logging in the User for UserId {UserId}.", request.UserId);
                throw new UserValidationException("An error occurred while logging in the User");
            }

            // Chat Server IP, Port, SessionID를 담아서 리턴
            var loginSuccessResponse = new LoginResponse
            {
                ServerIp = "127.0.0.1",
                ServerPort = "4242",
                SessionId = sessionId
            };

            return new UserResponse
            {
                Status = UserStatusCode.Success,
                Message = "User login successfully",
                Content = Any.Pack(loginSuccessResponse)
            };
        }

        public async Task<UserResponse> CreateUser(CreateUserRequest request)
        {
            var status = UserStatusCode.Failure;

            UserDto user = new UserDto
            {
                UserId = request.UserId,
                Password = PasswordHelper.HashPassword(request.Password),
                Username = request.Username,
                Email = request.Email,
                IsAlive = "Y"
            };

            status = await _userRepository.CreateUser(user);

            if (status != UserStatusCode.Success)
            {
                throw new UserValidationException("User creation failed");
            }

            return new UserResponse
            {
                Status = status,
                Message = "User created successfully",
                Content = Any.Pack(request)
            };
        }

        public async Task<UserResponse> UpdateUser(UpdateUserRequest request)
        {
            var user = new UserDto
            {
                UserId = request.UserId,
                Password = request.Password
            };

            (var status, var userInfo) = await _userRepository.ValidateUserCredentials(user);

            if (status != UserStatusCode.Success)
            {
                throw new UserValidationException("User not found or invalid credentials");
            }

            // 업데이트할 필드만 업데이트
            if (!string.IsNullOrEmpty(request.ToBePassword))
            {
                userInfo.Password = PasswordHelper.HashPassword(request.ToBePassword);
            }
            if (!string.IsNullOrEmpty(request.ToBeUsername))
            {
                userInfo.Username = request.ToBeUsername;
            }
            if (!string.IsNullOrEmpty(request.ToBeEmail))
            {
                userInfo.Email = request.ToBeEmail;
            }

            // 사용자 정보를 업데이트
            status = await _userRepository.UpdateUser(userInfo);
            if (status != UserStatusCode.Success)
            {
                throw new UserValidationException("Failed to update user");
            }

            return new UserResponse
            {
                Status = status,
                Message = "User updated successfully"
            };
        }


        public async Task<UserResponse> DeleteUser(DeleteUserRequest request)
        {
            var user = new UserDto
            {
                UserId = request.UserId,
                Password = request.Password
            };

            (var status, var userInfo) = await _userRepository.ValidateUserCredentials(user);

            if (status != UserStatusCode.Success)
            {
                throw new UserNotFoundException("User not found or invalid credentials");
            }

            // 소프트 삭제
            userInfo.IsAlive = "N";

            status = await _userRepository.DeleteUser(userInfo);
            if (status != UserStatusCode.Success)
            {
                throw new UserValidationException("Failed to delete user");
            }

            return new UserResponse
            {
                Status = status,
                Message = "User deleted successfully"
            };
        }

    }
}
