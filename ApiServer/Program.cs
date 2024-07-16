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

// API ��������Ʈ Ž�� ����ȭ���� ���� ���
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen();

builder.Services.AddSingleton<IUserWriteService, UserWriteService>();
builder.Services.AddSingleton<IUserReadService, UserReadService>();
builder.Services.AddSingleton<IUserRepository, UserRepositoryFromMysql>();
builder.Services.AddSingleton<ICacheRepository, CacheRepositoryFromRedis>();
builder.Services.AddSingleton<HStarContext, HStarContext>();

var app = builder.Build();

// Swagger�� ���� ȯ�濡���� Ȱ��ȭ
if (app.Environment.IsDevelopment())
{
    app.UseSwagger();
    app.UseSwaggerUI();
}

// ��� HTTP ��û�� HTTPS�� ���𷺼�
//app.UseHttpsRedirection();

app.UseAuthorization();

// ��Ʈ�ѷ��� ��������Ʈ�� ����
app.MapControllers();

app.Run();


// Ȯ�� �޼��� ����
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