#include "Udp.h"
#include "Motor.h"
#include "Sensor.h"
#include "BlockModel.h"
#include "esp_system.h"

#define BREAK 100

Motor motor;
Udp udp;
Sensor sensor;

char ssid[] = "ERS-AP";
char password[] = "robop0304";
IPAddress local_IP(192, 168, 1, 101); // 下3桁を101から機体ごとに連番で指定
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

int MOTOR_POWER_LOW = 60;
int MOTOR_POWER_MIDDLE = 80;
int MOTOR_POWER_HIGH = 100;

void reboot_task(void *pvParameters)
{
    while (1)
    {
        udp.recieve_packet();
        String packet = udp.get_packet_buffer();
        if (packet.startsWith("reboot-now"))
        {
            Serial.println("reboot-now");
            udp.disconnect();
            system_restart();
            delay(2000);
        }
        udp.clear_packet_buffer();
        delay(1000);
    }
}

void setup()
{
    Serial.begin(115200);

    Serial.println("setup start!");
    xTaskCreate(reboot_task, "reboot_task", 1024, NULL, 1, NULL);

    //Wifi設定
    udp.setup_udp(ssid, password, local_IP, gateway, subnet);
    udp.clear_packet_buffer();
    motor = Motor();
    sensor = Sensor();
}

void loop()
{
    udp.recieve_packet();
    String commands = udp.get_packet_buffer(); // コマンド文字列受け取り
    udp.clear_packet_buffer();
    if (commands.length() > 0)
    {
        Serial.println("get packet Datas");
        int model_size = 50; //BlockModelの配列のサイズ

        BlockModel block_models[model_size] = {}; //block_splitした後のBlockModelが入る
        block_split(block_models, commands);      //受信したコマンドを命令ごとに整形

        execute_command(block_models);

        //データ削除
        udp.clear_packet_buffer();
        memset(block_models, '\0', model_size);
        // memset(for_decomposed_models, '\0', model_size);

        //完了通知
        Serial.println("Done");
        char sendText[] = "Done";
        udp.send_data(udp.get_remote_ip(), sendText);

        motor.coast();
    }
}

int resistivityRead()
{
    int PIN_NUM = 33;
    int reading = analogRead(PIN_NUM);
    // MAX256の値で返す
    return reading / 128;
}

// flagは1を入れると左モーター,2で右モーターの値を返す
int generateSpeed(int flag, int app_speed)
{
    int resistivity_num = resistivityRead();
    int left_num = 16 - resistivity_num;
    int right_num = resistivity_num - 16;
    // -の値だったら0にする
    if (left_num < 0)
    {
        left_num = 0;
    }
    if (right_num < 0)
    {
        right_num = 0;
    }
    if (flag == 1)
    {
        return app_speed + left_num;
        // switch (app_speed)
        // {
        // // 最低速度
        // case 1:
        //     return MOTOR_POWER_LOW + left_num;
        // // 真ん中
        // case 2:
        //     return MOTOR_POWER_MIDDLE + left_num;
        // // 最速
        // case 3:
        //     return MOTOR_POWER_HIGH + left_num;
        // }
    }
    else if (flag == 2)
    {
        return app_speed + right_num;
        // switch (app_speed)
        // {
        // // 最低速度
        // case 1:
        //     return MOTOR_POWER_LOW + right_num;
        // // 真ん中
        // case 2:
        //     return MOTOR_POWER_MIDDLE + right_num;
        // // 最速
        // case 3:
        //     return MOTOR_POWER_HIGH + right_num;
        // }
    }
    return 0;
}

//udp通信で送られてきた文字列をsplitして、参照渡しでblock_modelsに返す
void block_split(BlockModel block_models[50], String text)
{
    int model_size = 50;
    String block_texts[model_size] = {};
    String tmp_text = text;
    for (int i = 0; i < text.length() / 13; i++)
    {
        block_texts[i] = tmp_text.substring(0, 13);
        tmp_text = tmp_text.substring(13, tmp_text.length());
    }

    for (int i = 0; i < model_size; i++)
    {
        if (block_texts[i].length() == 0)
        {
            block_models[i].set_block_state(0);
            block_models[i].set_left_speed(0);
            block_models[i].set_right_speed(0);
            block_models[i].set_time(0);
            break;
        }
        int block_state = block_texts[i].substring(0, 4).toInt();
        int block_left_speed = 0;
        switch (block_state)
        {
        case 7:
            block_left_speed = block_texts[i].substring(4, 7).toInt();
            break;
        default:
            block_left_speed = generateSpeed(1, block_texts[i].substring(4, 7).toInt());
            break;
        }
        int block_right_speed = generateSpeed(2, block_texts[i].substring(7, 10).toInt());
        int block_time = block_texts[i].substring(10, 13).toInt();

        block_models[i].set_block_state(block_state);
        block_models[i].set_left_speed(block_left_speed);
        block_models[i].set_right_speed(block_right_speed);
        block_models[i].set_time(block_time);
    }
}

BlockModel block_state_change(BlockModel model)
{
    switch (model.get_block_state())
    {
    case 101:
    case 201:
        model.set_block_state(1);
        break;

    case 102:
    case 202:
        model.set_block_state(2);
        break;

    case 103:
    case 203:
        model.set_block_state(3);
        break;

    case 104:
    case 204:
        model.set_block_state(4);
        break;

    case 105:
    case 205:
        model.set_block_state(5);
        break;

    case 106:
    case 206:
        model.set_block_state(6);
        break;

    case 109:
    case 209:
        model.set_block_state(9);
        break;
    }
    return model;
}

bool is_incorrect_state(int state_num)
{
    bool is_undefined = false;

    switch (state_num)
    {
    case 1:
    case 101:
    case 201:
        //Serial.println("前進処理のブロック");
        break;

    case 2:
    case 102:
    case 202:
        //Serial.println("後退処理のブロック");
        break;

    case 3:
    case 103:
    case 203:
        //Serial.println("左回転処理のブロック");
        break;

    case 4:
    case 104:
    case 204:
        //Serial.println("右回転処理のブロック");
        break;

    case 5:
    case 105:
    case 205:
        //Serial.println("forスタートのブロック");
        break;

    case 6:
    case 106:
    case 206:
        //Serial.println("forエンドのブロック");
        break;

    case 7:
        //Serial.println("ifスタートのブロック");
        break;

    case 8:
        //Serial.println("ifエンドのブロック");
        break;

    case 9:
    case 109:
    case 209:
        //Serial.println("breakのブロック");
        break;

    default:
        //Serial.println("コマンド終了");
        is_undefined = true;
        break;
    }

    return is_undefined;
}

void execute_command(BlockModel block_models[50])
{
    int model_size = 50;

    for (int i = 0; i < model_size; i++)
    {
        bool is_undefined_state = is_incorrect_state(block_models[i].get_block_state());
        bool is_loop_start = (block_models[i].get_block_state() == 5);
        bool is_if_start = block_models[i].get_block_state() == 7;

        if (is_undefined_state)
        {
            break;
        }
        else if (is_loop_start)
        {
            //BUG breakを使用した際にここから無限ループが起きている
            int loop_start_index = i + 1;  //ループスタートブロックの次のブロックのindex
            int loop_count = block_models[i].get_loop_count();
            int loop_end_index = find_loop_scope(block_models, loop_start_index, loop_count);
            i = loop_end_index;
        }
        else if (is_if_start)
        {
            int if_start_index = i;
            int if_end_index = find_if_scope(block_models, if_start_index);
            i = if_end_index;
        }
        else
        {
            motor.run_motor(block_models[i]);
        }
    }
}

int find_loop_scope(BlockModel block_models[50], int loop_start_index, int loop_count)
{
    int model_size = 50;
    BlockModel range_loop_blocks[model_size] = {};
    int j = 0; //range_loop_blocksのindex管理値

    for (int i = loop_start_index; i < model_size; i++)
    {
        bool is_undefined_state = is_incorrect_state(block_models[i].get_block_state());
        bool is_loop_end = (block_models[i].get_block_state() == 6 || block_models[i].get_block_state() == 106 || block_models[i].get_block_state() == 206);

        if (is_undefined_state)
        {
            break;
        }
        else if (is_loop_end)
        {
            int loop_end_index = i;
            execute_loop_command(range_loop_blocks, loop_count);

            memset(range_loop_blocks, '\0', model_size);
            return loop_end_index;
        }
        else
        {
            range_loop_blocks[j] = block_models[i];
            j++;
        }
    }

    return 0;
}

void execute_loop_command(BlockModel range_loop_blocks[50], int loop_count)
{
    int model_size = 50;

    for (int i = 0; i < loop_count; i++)
    {
        for (int j = 0; j < model_size; j++)
        {
            bool is_undefined_state = is_incorrect_state(range_loop_blocks[j].get_block_state());
            bool is_if_start = (range_loop_blocks[j].get_block_state() == 7 || range_loop_blocks[j].get_block_state() == 107 || range_loop_blocks[j].get_block_state() == 207);

            if (is_undefined_state)
            {
                break;
            }
            else if (is_if_start)
            {
                int if_start_index = j;
                int if_end_index = find_if_scope(range_loop_blocks, if_start_index);
                if(if_end_index == BREAK)
                {
                    return; //breakなら関数処理終了
                }
                j = if_end_index;
            }
            else
            {
                motor.run_motor(range_loop_blocks[j]);
            }
        }
    }
}

int find_if_scope(BlockModel block_models[50], int if_start_index)
{
    int model_size = 50;
    BlockModel true_blocks[model_size] = {};
    int true_count = 0;
    BlockModel false_blocks[model_size] = {};
    int false_count = 0;
    int if_end_index = 0;

    for (int i = if_start_index + 1; i < model_size; i++)
    {
        bool is_undefined_state = is_incorrect_state(block_models[i].get_block_state());
        bool is_if_end = block_models[i].get_block_state() == 8;
        bool is_true_models = (100 < block_models[i].get_block_state() && block_models[i].get_block_state() < 200);
        bool is_false_models = (200 < block_models[i].get_block_state() && block_models[i].get_block_state() < 300);

        if (is_undefined_state)
        {
            break;
        }
        else if (is_if_end)
        {
            if_end_index = i;
        }
        else if (is_true_models)
        {
            true_blocks[true_count] = block_models[i];
            true_blocks[true_count] = block_state_change(true_blocks[true_count]);

            true_count++;
        }
        else if (is_false_models)
        {
            false_blocks[false_count] = block_models[i];
            false_blocks[false_count] = block_state_change(false_blocks[false_count]);

            false_count++;
        }
    }

    if (block_models[if_start_index].get_if_comparison() == 1)
    {
        if (block_models[if_start_index].get_if_threshold() < sensor.getDistance())
        {
            for (int j = 0; j <= true_count; j++)
            {
                bool is_undefined_state = is_incorrect_state(true_blocks[j].get_block_state());
                bool is_loop_start = (true_blocks[j].get_block_state() == 5);
                bool is_break = true_blocks[j].get_block_state() == 9;
                if (is_undefined_state)
                {
                    break;
                }
                else if (is_break)
                {
                    return BREAK;
                }
                else if (is_loop_start)
                {
                    int loop_start_index = j + 1;
                    int loop_count = true_blocks[j].get_loop_count();
                    int loop_end_index = find_loop_scope(true_blocks, loop_start_index, loop_count);
                    j = loop_end_index;
                }
                else
                {
                    motor.run_motor(true_blocks[j]);
                }
            }
            memset(true_blocks, '\0', model_size);
            memset(false_blocks, '\0', model_size);
            return if_end_index;
        }
        else
        {
            for (int j = 0; j <= false_count; j++)
            {
                bool is_undefined_state = is_incorrect_state(false_blocks[j].get_block_state());
                bool is_loop_start = (false_blocks[j].get_block_state() == 5);
                bool is_break = false_blocks[j].get_block_state() == 9;
                if (is_undefined_state)
                {
                    break;
                }
                else if (is_break)
                {
                    return BREAK;
                }
                else if (is_loop_start)
                {
                    int loop_start_index = j + 1;
                    int loop_count = false_blocks[j].get_loop_count();
                    int loop_end_index = find_loop_scope(false_blocks, loop_start_index, loop_count);
                    j = loop_end_index;
                }
                else
                {
                    motor.run_motor(false_blocks[j]);
                }
            }
            memset(true_blocks, '\0', model_size);
            memset(false_blocks, '\0', model_size);
            return if_end_index;
        }
    }
    else if (block_models[if_start_index].get_if_comparison() == 2)
    {
        if (block_models[if_start_index].get_if_threshold() > sensor.getDistance())
        {
            for (int j = 0; j <= true_count; j++)
            {
                bool is_undefined_state = is_incorrect_state(true_blocks[j].get_block_state());
                bool is_loop_start = (true_blocks[j].get_block_state() == 5);
                bool is_break = true_blocks[j].get_block_state() == 9;
                if (is_undefined_state)
                {
                    break;
                }
                else if (is_break)
                {
                    return BREAK;
                }
                else if (is_loop_start)
                {
                    int loop_start_index = j + 1;
                    int loop_count = true_blocks[j].get_loop_count();
                    int loop_end_index = find_loop_scope(true_blocks, loop_start_index, loop_count);
                    j = loop_end_index;
                }
                else
                {
                    motor.run_motor(true_blocks[j]);
                }
            }
            memset(true_blocks, '\0', model_size);
            memset(false_blocks, '\0', model_size);
            return if_end_index;
        }
        else
        {
            for (int j = 0; j <= false_count; j++)
            {
                bool is_undefined_state = is_incorrect_state(false_blocks[j].get_block_state());
                bool is_loop_start = (false_blocks[j].get_block_state() == 5);
                bool is_break = false_blocks[j].get_block_state() == 9;
                if (is_undefined_state)
                {
                    break;
                }
                else if (is_break)
                {
                    return BREAK;
                }
                else if (is_loop_start)
                {
                    int loop_start_index = j + 1;
                    int loop_count = false_blocks[j].get_loop_count();
                    int loop_end_index = find_loop_scope(false_blocks, loop_start_index, loop_count);
                    j = loop_end_index;
                }
                else
                {
                    motor.run_motor(false_blocks[j]);
                }
            }
            memset(true_blocks, '\0', model_size);
            memset(false_blocks, '\0', model_size);
            return if_end_index;
        }
    }
    return 0;
}
