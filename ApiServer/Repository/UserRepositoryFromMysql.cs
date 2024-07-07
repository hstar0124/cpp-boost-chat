using ApiServer.Model;
using ApiServer.Model.Entity;
using ApiServer.Repository.Context;
using LoginApiServer.Model;
using LoginApiServer.Repository.Interface;
using LoginApiServer.Utils;
using Microsoft.EntityFrameworkCore;

namespace ApiServer.Repository
{
    public class UserRepositoryFromMysql : IUserRepository
    {
        private readonly ILogger<UserRepositoryFromMysql> _logger;
        private readonly HStarContext _context;

        public UserRepositoryFromMysql(ILogger<UserRepositoryFromMysql> logger, HStarContext hStarContext)
        {
            _logger = logger;
            _context = hStarContext;
        }

        public async Task<UserStatusCode> CreateUser(UserDto userDto)
        {
            // 아이디가 이미 존재하는 경우
            UserEntity alreadyUser = await _context.User.SingleOrDefaultAsync(x => x.UserId == userDto.UserId);
            if (alreadyUser != null)
            {
                _logger.LogError("User creation failed for UserId {UserId}. UserId already exists.", userDto.UserId);
                return UserStatusCode.UserIdAlreadyExists;
            }

            try
            {
                UserEntity userEntity = new UserEntity
                {
                    UserId = userDto.UserId,
                    Password = userDto.Password,
                    Username = userDto.Username,
                    Email = userDto.Email,
                    IsAlive = userDto.IsAlive,
                };

                _context.User.Add(userEntity);
                await _context.SaveChangesAsync();

                _logger.LogInformation("User with UserId {UserId} created successfully.", userDto.UserId);
                return UserStatusCode.Success;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "User creation failed for UserId {UserId}.", userDto.UserId);
                return UserStatusCode.Failure;
            }
        }



        public async Task<(UserStatusCode, GetUserResponse)> GetUserFromUserid(string id)
        {
            // 단순한 Read 작업은 Track을 끔으로 성능 향상
            UserEntity storedUser = await _context.User.AsNoTracking().SingleOrDefaultAsync(x => x.UserId == id);

            // 유저가 존재하지 않은 경우
            if (storedUser == null)
            {
                _logger.LogError("User not exists :: {UserId}", id);
                return (UserStatusCode.UserNotExists, null);
            }

            // 삭제된 유저인 경우
            if (storedUser.IsAlive == "N")
            {
                _logger.LogError("User not exists :: {UserId}", id);
                return (UserStatusCode.UserNotExists, null);
            }

            // DB에서 가져온 값을 그대로 유저에게 넘겨주지 않도록 주의
            var userResponse = new GetUserResponse
            {
                UserId = storedUser.UserId,
                Username = storedUser.Username,
                Email = storedUser.Email
            };

            _logger.LogInformation("User retrieval successfully :: {UserId}", id);
            return (UserStatusCode.Success, userResponse);
        }

        public async Task<(UserStatusCode, UserDto)> ValidateUserCredentials(UserDto userInfo)
        {
            // 단순한 Read 작업은 Track을 끔으로 성능 향상
            UserEntity storedUser = await _context.User.AsNoTracking().SingleOrDefaultAsync(x => x.UserId == userInfo.UserId);
            
            // 유저가 존재하지 않은 경우
            if (storedUser == null)
            {
                _logger.LogError("User not exists :: {UserId}", userInfo.UserId);
                return (UserStatusCode.UserNotExists, null);
            }

            // 삭제된 유저인 경우
            if(storedUser.IsAlive == "N")
            {
                _logger.LogError("User not exists :: {UserId}", userInfo.UserId);
                return (UserStatusCode.UserNotExists, null);
            }

            // 비밀번호가 틀린 경우
            if (!PasswordHelper.VerifyPassword(storedUser.Password, userInfo.Password))
            {
                _logger.LogError("Incorrect password for user :: {UserId}", userInfo.UserId);
                return (UserStatusCode.DifferentPassword, null);
            }

            UserDto result = new UserDto
            {
                Id = storedUser.Id,
                UserId = storedUser.UserId,
                Username = storedUser.Username,
                Email = storedUser.Email,
                IsAlive = storedUser.IsAlive,
            };

            _logger.LogInformation("User login successfully :: {UserId}", userInfo.UserId);
            return (UserStatusCode.Success, result);
        }

        public async Task<UserStatusCode> UpdateUser(UserDto userDto)
        {
            try
            {
                UserEntity userEntity = await _context.User.SingleOrDefaultAsync(x => x.UserId == userDto.UserId);

                if (userEntity == null)
                {
                    _logger.LogInformation("User update failed {UserId}", userDto.UserId);
                    return UserStatusCode.Failure;
                }

                userEntity.Password = userDto.Password;
                userEntity.Username = userDto.Username;
                userEntity.Email = userDto.Email;
                userEntity.IsAlive = userDto.IsAlive;

                _context.User.Update(userEntity);
                await _context.SaveChangesAsync();

                _logger.LogInformation("User with UserId {UserId} updated successfully.", userDto.UserId);
                return UserStatusCode.Success;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "User update failed for UserId {UserId}.", userDto.UserId);
                return UserStatusCode.Failure;
            }
        }

        public async Task<UserStatusCode> DeleteUser(UserDto userDto)
        {
            try
            {
                UserEntity userEntity = await _context.User.SingleOrDefaultAsync(x => x.UserId == userDto.UserId);

                if (userEntity == null)
                {
                    _logger.LogInformation("User update failed {UserId}", userDto.UserId);
                    return UserStatusCode.Failure;
                }

                userEntity.IsAlive = userDto.IsAlive;

                _context.User.Update(userEntity);
                await _context.SaveChangesAsync();

                _logger.LogInformation("User with UserId {UserId} deleted successfully.", userDto.UserId);
                return UserStatusCode.Success;
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "User delete failed for UserId {UserId}.", userDto.UserId);
                return UserStatusCode.Failure;
            }
        }
    }
}
