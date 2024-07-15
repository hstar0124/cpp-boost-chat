using Google.Protobuf.WellKnownTypes;
using LoginApiServer.Model;
using LoginApiServer.Service.Interface;
using LoginApiServer.Utils;
using Microsoft.AspNetCore.Mvc;

namespace LoginApiServer.Controllers
{
    [ApiController]
    [Route("[controller]/[action]")]
    public class UserController : ControllerBase
    {
        private readonly ILogger<UserController> _logger;
        // CQRS 패턴 적용
        // 명령을 처리하는 책임과 조회를 처리하는 책임을 분리
        private readonly IUserWriteService _userWriteService;
        private readonly IUserReadService _userReadService;

        public UserController(ILogger<UserController> logger, IUserWriteService userWriteService, IUserReadService userReadService)
        {
            _logger = logger;
            _userWriteService = userWriteService;
            _userReadService = userReadService;
        }

        [HttpGet]
        public async Task<UserResponse> GetUser([FromQuery]string userId)
        {           
            try
            {
                var response = await _userReadService.GetUserFromUserid(userId);
                return response;
            }
            catch (Exception ex)
            {
                return ProtobufResultHelper.CreateErrorResult(UserStatusCode.ServerError, $"An error occurred while getting the User: {ex.Message}");
            }
        }

        // 유저 생성
        [HttpPost]
        public async Task<UserResponse> Create([FromBody] CreateUserRequest request)
        {
            try
            {
                var response = await _userWriteService.CreateUser(request);
                return response;
            }
            catch (Exception ex)
            {
                return ProtobufResultHelper.CreateErrorResult(UserStatusCode.ServerError, $"An error occurred while creating the User: {ex.Message}");
            }
        }

        // 유저 로그인
        [HttpPost]
        public async Task<UserResponse> Login([FromBody] LoginRequest request)
        {
            try
            {
                var response = await _userWriteService.LoginUser(request);
                return response;
            }
            catch (Exception ex)
            {
                return ProtobufResultHelper.CreateErrorResult(UserStatusCode.ServerError, $"An error occurred while loging the User: {ex.Message}");
            }
        }

        // 유저 변경
        [HttpPost]
        public async Task<UserResponse> Update([FromBody] UpdateUserRequest request)
        {
            try
            {
                var response = await _userWriteService.UpdateUser(request);

                return response;

            }
            catch (Exception ex)
            {

                return ProtobufResultHelper.CreateErrorResult(UserStatusCode.ServerError, $"An error occurred while updating the User: {ex.Message}");
            }
        }

        // 유저 삭제
        [HttpPost]
        public async Task<UserResponse> Delete([FromBody] DeleteUserRequest request)
        {
            try
            {
                var response = await _userWriteService.DeleteUser(request);

                return response;
            }
            catch (Exception ex)
            {

                return ProtobufResultHelper.CreateErrorResult(UserStatusCode.ServerError, $"An error occurred while delete the User: {ex.Message}");
            }

        }
    }
}
