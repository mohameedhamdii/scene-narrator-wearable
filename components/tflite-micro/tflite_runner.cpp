#include "tflite_runner.h"
#include "jpeg_decode.h"
#include "imagenet_labels.h"

#include "esp_log.h"
#include "esp_heap_caps.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

static const char *TAG = "tflite";

static constexpr int kArenaSize = 512 * 1024;
static constexpr float kDetThreshold =0.5f;

struct tflite_runner{
    const tflite::Model *model;
    tflite::MicroInterpreter *interpreter;
    uint8_t *arena;
    int input_h;
    int input_w;


    int input_c;
    bool is_quantized;
    bool is_detector;
    bool is_recogniser;

};

static bool is_tflite_model(const uint8_t *data, size_t size)
{
    if(!data || size < 8) {
        return false;
    }


    return data[0] == 'T' && data[1] == 'F' && data[2] == 'L' && data[3] == '3';
}

static tflite_runner_t *create_interpreter(const uint8_t *model_data,size_t model_size)
{
    if (!is_tflite_model(model_data, model_size)){
        ESP_LOGW(TAG,"Not a TFLite flatbuffer");
        return nullptr;


    }

    auto *runner = static_cast<tflite_runner_t *>(calloc(1, sizeof(tflite_runner_t)));
    if (!runner) {
        return nullptr;
    }

    runner->model = tflite::GetModel(model_data);
    if (runner->model->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGW(TAG,"Unsupported TFLite schema %lu", (unsigned long)runner->model->version());

        free(runner);
        return nullptr;
    }

    runner->arena=static_cast<uint8_t *>(
        heap_caps_malloc(kArenaSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!runner->arena) {
        free(runner);
        return nullptr;
    }

    static tflite::MicroMutableOpResolver<32> resolver;
    resolver.AddConv2D();
    resolver.AddDepthwiseConv2D();
    resolver.AddFullyConnected();
    resolver.AddSoftmax();
    resolver.AddReshape();
    resolver.AddAveragePool2D();
    resolver.AddMaxPool2D();
    resolver.AddAdd();
    resolver.AddMul();
    resolver.AddQuantize();
    resolver.AddDequantize();


    resolver.AddConcatenation();
    resolver.AddPad();
    resolver.AddStridedSlice();
    resolver.AddTranspose();
    resolver.AddResizeNearestNeighbor();
    resolver.AddLogistic();
    resolver.AddRelu();
    resolver.AddRelu6();
    resolver.AddTanh();
    resolver.AddMean();
    resolver.AddSub();
    resolver.AddSplit();
    resolver.AddPack();
    resolver.AddShape();


    resolver.AddCast();

    runner->interpreter = new tflite::MicroInterpreter(
        runner->model, resolver, runner->arena, kArenaSize);
    if(runner->interpreter->AllocateTensors()!= kTfLiteOk) {
        ESP_LOGE(TAG, "AllocateTensors failed");
        delete runner->interpreter;
        heap_caps_free(runner->arena);
        free(runner);
        return nullptr;
    }

    TfLiteTensor *input = runner->interpreter->input(0);
    if (input->dims->size >= 4) {
        runner->input_h=input->dims->data[1];
        runner->input_w=input->dims->data[2];
        runner->input_c=input->dims->data[3];
    }else if(input->dims->size == 3) {
        runner->input_h = input->dims->data[1];
        runner->input_w = input->dims->data[2];
        runner->input_c =1;
    } else {

        runner->input_h= 224;
        runner->input_w = 224;
        runner->input_c =3;
    }

    runner->is_quantized= (input->type == kTfLiteInt8 || input->type == kTfLiteUInt8);

    int outputs = runner->interpreter->outputs_size();
    if (outputs >= 4){
        runner->is_detector = true;

    } else if(input->dims->size >= 3 && runner->input_h <= 64) {
        runner->is_recogniser=true;
    }

    ESP_LOGI(TAG, "Model ready: %dx%dx%d quant=%d det=%d rec=%d",
             runner->input_w, runner->input_h, runner->input_c,
             runner->is_quantized,runner->is_detector,runner->is_recogniser);
    return runner;
}

tflite_runner_t *tflite_runner_create(const uint8_t *model_data,size_t model_size)
{
    return create_interpreter(model_data, model_size);
}

void tflite_runner_destroy(tflite_runner_t *runner)
{
    if(!runner) {


        return;
    }
    delete runner->interpreter;
    if (runner->arena) {
        heap_caps_free(runner->arena);
    }
    free(runner);
}

static void fill_input_rgb(tflite_runner_t *runner,const rgb_image_t *img)
{
    TfLiteTensor *input = runner->interpreter->input(0);


    int h = runner->input_h;

    int w = runner->input_w;

    rgb_image_t resized = {0};
    const uint8_t *src=img->rgb;
    if (img->width != w || img->height != h) {
        resized.rgb = static_cast<uint8_t *>(
            heap_caps_malloc((size_t)w * h * 3,MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));


        if(!resized.rgb){
            return;
        }
        for (int y = 0; y < h; y++) {
            int sy = y * img->height / h;
            for (int x = 0; x < w; x++) {
                int sx = x * img->width / w;
                const uint8_t *sp = img->rgb + (sy * img->width + sx) * 3;

                uint8_t *dp= resized.rgb +(y * w + x)* 3;
                dp[0] =sp[0];
                dp[1]= sp[1];
                dp[2] = sp[2];
            }
        }
        resized.width= w;
        resized.height= h;
        src=resized.rgb;


    }

    if (runner->is_quantized){
        int zero= input->params.zero_point;
        float scale = input->params.scale;
        int8_t *out = input->data.int8;
        for(int y= 0; y < h; y++) {
            for(int x =0; x < w; x++){
                const uint8_t *p = src + (y * w + x) * 3;
                for (int c = 0; c < 3; c++) {
                    float norm = (static_cast<float>(p[c]) - 127.5f) / 127.5f;
                    int q = static_cast<int>(std::round(norm / scale) + zero);
                    out[(y * w + x) * 3 + c] = static_cast<int8_t>(std::clamp(q, -128, 127));
                }
            }
        }
    } else {
        float *out = input->data.f;

        for(int y= 0;y < h;y++){
            for (int x = 0; x < w; x++) {
                const uint8_t *p=src +(y * w + x) * 3;
                for (int c = 0; c < 3; c++) {
                    out[(y * w + x) * 3 + c] = (static_cast<float>(p[c]) - 127.5f) / 127.5f;
                }
            }
        }
    }

    if (resized.rgb && resized.rgb != img->rgb) {

        heap_caps_free(resized.rgb);

    }
}

static bool invoke_rgb(tflite_runner_t *runner,const rgb_image_t *img)
{
    fill_input_rgb(runner, img);
    return runner->interpreter->Invoke()== kTfLiteOk;
}

static void top_k_classes(TfLiteTensor *output, int k, int *indices, float *scores)
{
    int count =1;
    for (int i = 0; i < output->dims->size; i++) {
        count *= output->dims->data[i];
    }

    std::vector<std::pair<float, int>> ranked;
    ranked.reserve(count);
    if (output->type == kTfLiteUInt8) {
        for (int i= 0;i < count; i++){
            ranked.push_back({output->data.uint8[i] / 255.0f,i});
        }
    } else if(output->type == kTfLiteInt8){
        for(int i =0;i < count; i++) {
            float v = (output->data.int8[i] - output->params.zero_point) * output->params.scale;
            ranked.push_back({v, i});


        }
    } else {
        for(int i=0;i < count;i++){
            ranked.push_back({output->data.f[i], i});
        }
    }

    std::partial_sort(ranked.begin(), ranked.begin() + k, ranked.end(),
                      [](const auto &a, const auto &b) {return a.first > b.first;});

    for (int i = 0; i < k; i++) {
        indices[i] =ranked[i].second;
        scores[i] = ranked[i].first;
    }
}

bool tflite_runner_describe_scene(tflite_runner_t *runner,
                                  const uint8_t *jpeg, size_t jpeg_len,
                                  char *caption, size_t caption_len)
{
    if (!runner || !jpeg || !caption || caption_len == 0 || runner->is_detector) {
        return false;
    }



    rgb_image_t img = {0};
    if(!jpeg_decode_to_rgb(jpeg,jpeg_len,&img)) {
        return false;
    }

    bool ok = invoke_rgb(runner,&img);
    if(!ok) {
        jpeg_decode_free(&img);
        return false;
    }

    TfLiteTensor *output= runner->interpreter->output(0);
    int idx[3] = {0};
    float scores[3]={0};
    top_k_classes(output,3,idx,scores);

    const char *a=(idx[0] <(int)IMAGENET_LABEL_COUNT) ? IMAGENET_LABELS[idx[0]] : "object";
    const char *b = (idx[1] < (int)IMAGENET_LABEL_COUNT) ? IMAGENET_LABELS[idx[1]] : "item";
    snprintf(caption,caption_len,
             "I see a %s and a %s nearby.",a, b);



    jpeg_decode_free(&img);
    ESP_LOGI(TAG, "Scene: %s (%.2f, %.2f)", caption, scores[0], scores[1]);
    return true;
}

static float read_output_at(TfLiteTensor *tensor, int index)
{
    if (tensor->type == kTfLiteUInt8){
        return tensor->data.uint8[index]/ 255.0f;
    }
    if (tensor->type == kTfLiteInt8) {
        return (tensor->data.int8[index] - tensor->params.zero_point) * tensor->params.scale;

    }
    return tensor->data.f[index];
}

bool tflite_runner_detect_text(tflite_runner_t *runner,
                               const uint8_t *jpeg,size_t jpeg_len,
                               tflite_text_box_t *boxes, int max_boxes,int *count)
{
    if (!runner || !jpeg || !boxes || !count || max_boxes <= 0) {
        return false;
    }
    *count = 0;


    if(!runner->is_detector){


        return false;
    }



    rgb_image_t img={0};
    if (!jpeg_decode_to_rgb_resized(jpeg, jpeg_len, runner->input_w,runner->input_h, &img)) {
        return false;
    }

    if (!invoke_rgb(runner, &img)) {
        jpeg_decode_free(&img);
        return false;
    }

    TfLiteTensor *scores =runner->interpreter->output(0);
    int map_h=scores->dims->data[1];
    int map_w= scores->dims->data[2];

    for (int y = 0; y < map_h && *count < max_boxes; y++) {
        for (int x=0; x < map_w && *count < max_boxes; x++){
            int idx =y * map_w + x;
            float score = read_output_at(scores, idx);
            if (score < kDetThreshold) {
                continue;


            }
            boxes[*count].x = x * img.width / map_w;
            boxes[*count].y= y * img.height / map_h;
            boxes[*count].w =img.width / map_w * 4;
            boxes[*count].h = img.height / map_h * 2;
            boxes[*count].score = score;

            (*count)++;
        }
    }

    jpeg_decode_free(&img);
    return *count > 0;
}

bool tflite_runner_recognise_text(tflite_runner_t *runner,
                                  const uint8_t *jpeg, size_t jpeg_len,
                                  const tflite_text_box_t *box,
                                  char *text, size_t text_len)
{
    if(!runner || !jpeg || !box || !text || text_len == 0){
        return false;
    }

    rgb_image_t img ={0};
    if(!jpeg_decode_to_rgb(jpeg,jpeg_len,&img)) {
        return false;
    }


    int x0= std::max(0,box->x);
    int y0 = std::max(0,box->y);
    int x1= std::min(img.width, box->x + box->w);
    int y1= std::min(img.height,box->y + box->h);
    int cw = x1 - x0;
    int ch=y1 - y0;
    if(cw <= 0 || ch <= 0) {
        jpeg_decode_free(&img);
        return false;
    }

    rgb_image_t crop = {0};
    crop.width = runner->input_w;
    crop.height= runner->input_h;
    crop.rgb =static_cast<uint8_t *>(
        heap_caps_malloc((size_t)crop.width * crop.height * 3,MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!crop.rgb) {
        jpeg_decode_free(&img);
        return false;
    }



    for(int y = 0;y < crop.height;y++){

        int sy= y0 + y * ch / crop.height;


        for (int x=0; x < crop.width; x++){
            int sx= x0 + x * cw / crop.width;
            const uint8_t *sp = img.rgb + (sy * img.width + sx) * 3;
            uint8_t *dp=crop.rgb + (y * crop.width + x)* 3;
            dp[0]= sp[0];
            dp[1] = sp[1];
            dp[2] = sp[2];
        }
    }

    jpeg_decode_free(&img);

    if(!invoke_rgb(runner, &crop)) {
        jpeg_decode_free(&crop);
        return false;
    }

    TfLiteTensor *output =runner->interpreter->output(0);
    int classes = output->dims->data[output->dims->size - 1];
    static const char charset[] ="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    size_t pos= 0;
    for(int t = 0;t < classes && pos + 1 < text_len;t++){
        int best =0;
        float best_score=-1e9f;
        int stride=output->dims->data[output->dims->size - 1];

        for(int c= 0; c <(int)strlen(charset);c++){
            float v= read_output_at(output,t * stride + c);
            if (v > best_score) {
                best_score = v;
                best =c;
            }
        }
        if(charset[best]!= ' ') {
            text[pos++] = charset[best];
        }
    }
    text[pos] = '\0';

    jpeg_decode_free(&crop);

    return pos > 0;
}
