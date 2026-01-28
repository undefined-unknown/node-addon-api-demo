# 译码器流程介绍


## 1.作用

### 1.1 六个译码工具脚本

**a.bmp_extract**

这一步是将几张图片分裂成几个toml小数组放在output/toml中

```powershell
cd ./src/1.bmp_extract
g++ -shared -o bmp_extract.dll bmp_extract.cpp -std=c++17 -DBMP_EXTRACT_EXPORTS
```

**b.toml_handle**

这一步是将上一步几个分裂开的toml合并，再通过查找config_files/color_to_number.toml生成output/toml/combined.toml
其中内容为所有维度的toml矩阵

```powershell
cd ./src/2.toml_handle
g++ -shared -o toml_handle.dll toml_handle.cpp -std=c++17 -DTOML_HANDLE_EXPORTS
```

**c.data_csv_handle**

这一步是将上一步的合并的全信息combined.toml串行化，按照纱线数量规划编织方向，分配行切换与纱嘴切换，并按照时序分布到output/csv/pixel_data.csv中

```powershell
cd ./src/3.data_csv_handle
g++ -shared -o data_csv_handle.dll data_csv_handle.cpp -std=c++17 -DDATA_CSV_HANDLE_EXPORTS 
```

**d.cmd_csv_handle**

这一步是将上一步串行化后的数据查找sema_to_cmd.toml、dumu_to_cmd.toml、line_switch_to_cmd.toml、luola_to_cmd.toml、post_action_to_cmd.tom、pre_action_to_cmd.toml、shaxian_switch_to_cmd.toml、zhenban_qianhou.toml
生成转化成命令之后的串行时序代码到output/csv/pixel_cmd.csv中

```powershell
cd ./src/4.cmd_csv_handle
g++ -shared -o cmd_csv_handle.dll cmd_csv_handle.cpp -std=c++17 -DCMD_CSV_HANDLE_EXPORTS  
```

**e.txt_generator**

这一步是将上一步转化好之后的串行数据排列到txt文本中，会生成两份文件，output/txt/cmd_simple.txt与output/txt/cmd_raw.txt

在"simple"文件中只是简单的将数据排列起来，在"在raw"文件中会插入每一段所属的index和具体的内容介绍

```powershell
cd ./src/5.txt_generator
g++ -shared -o txt_generator.dll txt_generator.cpp -std=c++17 -DTXT_GENERATOR_EXPORTS 
```

**f.txt_handle**

这一步是将上一步转化好之后的output/txt/cmd_simple.txt遍历其中的重复片段，并使用RS循环结构简化文本，最后生成压缩后的

output/txt/txt_compressed.txt

```powershell
cd ./src/6.txt_handle
g++ -shared -o txt_handle.dll txt_handle.cpp -std=c++17 -DTXT_HANDLE_EXPORTS 
```

## 2.(输入)配置文件

图片的等效替换内容，每种类型的机器都不一样，所以需要根据不同的机器区分不同的输入配置文件

在resources/config目录下应该分多级目录
暂定五针机器和七针机器

那么文件目录应为

/resources/config/niddle_5

/resources/config/niddle_7

每个子目录都应该包含如下文件

```
color_to_number.toml
dumu_to_cmd.toml
head_tail_cmd.toml
line_switch_to_cmd.toml
luola_to_cmd.toml
post_action_to_cmd.toml
pre_action_to_cmd.toml
sema_to_cmd.toml
shaxian_switch_to_cmd.toml
zhenban_qianhou.toml
```


并在留给前端的接口里面根据用户配置选择指定的机型目录

使用五针

configPath=join(process.resourcesPath, '/resources/config/niddle_5')

使用七针

configPath=join(process.resourcesPath, '/resources/config/niddle_7')


## 3.如何导入图片

将几个图层的文件绘制完毕或者经过前级处理后的最终文件(也就是要编译的文件)

放到/resources/input文件夹中

四个文件名称必须严格按照如下

```
dumu.bmp
luola.bmp
sema.bmp
shaxian.bmp
```

四个文件尺寸也必须严格一致

## 4.译码后的产出

根据现有代码会输出到

/resources/output目录下

其中会生成.toml和.csv文件为排查故障和中间问题所用暂时不用理会
重点关注三个文件

cmd_compressed.txt、cmd_raw.txt、cmd_simple.txt



cmd_simple.txt为生成的指令文件未经过压缩

cmd_raw.txt为生成的带注释的指令文件，也是排查问题用的

cmd_compressed.txt为压缩后的不带注释的指令文件，可以直接发给机器使用
