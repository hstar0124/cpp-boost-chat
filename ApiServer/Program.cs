using ApiServer.Filters;
using ApiServer.Repository;
using ApiServer.Repository.Context;
using LoginApiServer.Formatter;
using LoginApiServer.Repository;
using LoginApiServer.Repository.Interface;
using LoginApiServer.Service;
using LoginApiServer.Service.Interface;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddControllers(option => option.Filters.Add<GlobalExceptionFilter>())
    .AddProtobufFormatters();

// API 엔드포인트 탐색 문서화위한 서비스 등록
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen();

builder.Services.AddSingleton<IUserWriteService, UserWriteService>();
builder.Services.AddSingleton<IUserReadService, UserReadService>();
builder.Services.AddSingleton<IUserRepository, UserRepositoryFromMysql>();
builder.Services.AddSingleton<ICacheRepository, CacheRepositoryFromRedis>();
builder.Services.AddSingleton<HStarContext, HStarContext>();

var app = builder.Build();

// Swagger가 개발 환경에서만 활성화
if (app.Environment.IsDevelopment())
{
    app.UseSwagger();
    app.UseSwaggerUI();
}

// 모든 HTTP 요청을 HTTPS로 리디렉션
//app.UseHttpsRedirection();

app.UseAuthorization();

// 컨트롤러를 엔드포인트로 매핑
app.MapControllers();

app.Run();


// 확장 메서드 정의
public static class MvcBuilderExtensions
{
    public static IMvcBuilder AddProtobufFormatters(this IMvcBuilder builder)
    {
        return builder.AddMvcOptions(options =>
        {
            options.InputFormatters.Insert(0, new ProtobufInputFormatter());
            options.OutputFormatters.Insert(0, new ProtobufOutputFormatter());
        });
    }
}