#pragma once

#include "vivid/types.h"
#include <vector>

namespace goom::utils::colordata
{

// clang-format off
static const std::vector<vivid::srgb_t> RdGy
{
  {   0.40392f,   0.00000f,   0.12157f },
  {   0.41546f,   0.00369f,   0.12341f },
  {   0.42699f,   0.00738f,   0.12526f },
  {   0.43852f,   0.01107f,   0.12710f },
  {   0.45006f,   0.01476f,   0.12895f },
  {   0.46159f,   0.01845f,   0.13080f },
  {   0.47313f,   0.02215f,   0.13264f },
  {   0.48466f,   0.02584f,   0.13449f },
  {   0.49619f,   0.02953f,   0.13633f },
  {   0.50773f,   0.03322f,   0.13818f },
  {   0.51926f,   0.03691f,   0.14002f },
  {   0.53080f,   0.04060f,   0.14187f },
  {   0.54233f,   0.04429f,   0.14371f },
  {   0.55386f,   0.04798f,   0.14556f },
  {   0.56540f,   0.05167f,   0.14740f },
  {   0.57693f,   0.05536f,   0.14925f },
  {   0.58847f,   0.05905f,   0.15110f },
  {   0.60000f,   0.06275f,   0.15294f },
  {   0.61153f,   0.06644f,   0.15479f },
  {   0.62307f,   0.07013f,   0.15663f },
  {   0.63460f,   0.07382f,   0.15848f },
  {   0.64614f,   0.07751f,   0.16032f },
  {   0.65767f,   0.08120f,   0.16217f },
  {   0.66920f,   0.08489f,   0.16401f },
  {   0.68074f,   0.08858f,   0.16586f },
  {   0.69227f,   0.09227f,   0.16770f },
  {   0.70081f,   0.09965f,   0.17124f },
  {   0.70634f,   0.11073f,   0.17647f },
  {   0.71188f,   0.12180f,   0.18170f },
  {   0.71742f,   0.13287f,   0.18693f },
  {   0.72295f,   0.14394f,   0.19216f },
  {   0.72849f,   0.15502f,   0.19739f },
  {   0.73403f,   0.16609f,   0.20261f },
  {   0.73956f,   0.17716f,   0.20784f },
  {   0.74510f,   0.18824f,   0.21307f },
  {   0.75063f,   0.19931f,   0.21830f },
  {   0.75617f,   0.21038f,   0.22353f },
  {   0.76171f,   0.22145f,   0.22876f },
  {   0.76724f,   0.23253f,   0.23399f },
  {   0.77278f,   0.24360f,   0.23922f },
  {   0.77832f,   0.25467f,   0.24444f },
  {   0.78385f,   0.26574f,   0.24967f },
  {   0.78939f,   0.27682f,   0.25490f },
  {   0.79493f,   0.28789f,   0.26013f },
  {   0.80046f,   0.29896f,   0.26536f },
  {   0.80600f,   0.31003f,   0.27059f },
  {   0.81153f,   0.32111f,   0.27582f },
  {   0.81707f,   0.33218f,   0.28105f },
  {   0.82261f,   0.34325f,   0.28627f },
  {   0.82814f,   0.35433f,   0.29150f },
  {   0.83368f,   0.36540f,   0.29673f },
  {   0.83922f,   0.37647f,   0.30196f },
  {   0.84383f,   0.38708f,   0.31011f },
  {   0.84844f,   0.39769f,   0.31826f },
  {   0.85306f,   0.40830f,   0.32641f },
  {   0.85767f,   0.41892f,   0.33456f },
  {   0.86228f,   0.42953f,   0.34271f },
  {   0.86690f,   0.44014f,   0.35087f },
  {   0.87151f,   0.45075f,   0.35902f },
  {   0.87612f,   0.46136f,   0.36717f },
  {   0.88074f,   0.47197f,   0.37532f },
  {   0.88535f,   0.48258f,   0.38347f },
  {   0.88997f,   0.49319f,   0.39162f },
  {   0.89458f,   0.50381f,   0.39977f },
  {   0.89919f,   0.51442f,   0.40792f },
  {   0.90381f,   0.52503f,   0.41607f },
  {   0.90842f,   0.53564f,   0.42422f },
  {   0.91303f,   0.54625f,   0.43237f },
  {   0.91765f,   0.55686f,   0.44052f },
  {   0.92226f,   0.56747f,   0.44867f },
  {   0.92687f,   0.57809f,   0.45682f },
  {   0.93149f,   0.58870f,   0.46498f },
  {   0.93610f,   0.59931f,   0.47313f },
  {   0.94072f,   0.60992f,   0.48128f },
  {   0.94533f,   0.62053f,   0.48943f },
  {   0.94994f,   0.63114f,   0.49758f },
  {   0.95456f,   0.64175f,   0.50573f },
  {   0.95755f,   0.65121f,   0.51511f },
  {   0.95894f,   0.65952f,   0.52572f },
  {   0.96032f,   0.66782f,   0.53633f },
  {   0.96171f,   0.67612f,   0.54694f },
  {   0.96309f,   0.68443f,   0.55755f },
  {   0.96448f,   0.69273f,   0.56817f },
  {   0.96586f,   0.70104f,   0.57878f },
  {   0.96724f,   0.70934f,   0.58939f },
  {   0.96863f,   0.71765f,   0.60000f },
  {   0.97001f,   0.72595f,   0.61061f },
  {   0.97140f,   0.73426f,   0.62122f },
  {   0.97278f,   0.74256f,   0.63183f },
  {   0.97416f,   0.75087f,   0.64245f },
  {   0.97555f,   0.75917f,   0.65306f },
  {   0.97693f,   0.76747f,   0.66367f },
  {   0.97832f,   0.77578f,   0.67428f },
  {   0.97970f,   0.78408f,   0.68489f },
  {   0.98108f,   0.79239f,   0.69550f },
  {   0.98247f,   0.80069f,   0.70611f },
  {   0.98385f,   0.80900f,   0.71672f },
  {   0.98524f,   0.81730f,   0.72734f },
  {   0.98662f,   0.82561f,   0.73795f },
  {   0.98800f,   0.83391f,   0.74856f },
  {   0.98939f,   0.84221f,   0.75917f },
  {   0.99077f,   0.85052f,   0.76978f },
  {   0.99216f,   0.85882f,   0.78039f },
  {   0.99246f,   0.86436f,   0.78900f },
  {   0.99277f,   0.86990f,   0.79762f },
  {   0.99308f,   0.87543f,   0.80623f },
  {   0.99339f,   0.88097f,   0.81484f },
  {   0.99369f,   0.88651f,   0.82345f },
  {   0.99400f,   0.89204f,   0.83206f },
  {   0.99431f,   0.89758f,   0.84068f },
  {   0.99462f,   0.90311f,   0.84929f },
  {   0.99493f,   0.90865f,   0.85790f },
  {   0.99523f,   0.91419f,   0.86651f },
  {   0.99554f,   0.91972f,   0.87512f },
  {   0.99585f,   0.92526f,   0.88374f },
  {   0.99616f,   0.93080f,   0.89235f },
  {   0.99646f,   0.93633f,   0.90096f },
  {   0.99677f,   0.94187f,   0.90957f },
  {   0.99708f,   0.94740f,   0.91819f },
  {   0.99739f,   0.95294f,   0.92680f },
  {   0.99769f,   0.95848f,   0.93541f },
  {   0.99800f,   0.96401f,   0.94402f },
  {   0.99831f,   0.96955f,   0.95263f },
  {   0.99862f,   0.97509f,   0.96125f },
  {   0.99892f,   0.98062f,   0.96986f },
  {   0.99923f,   0.98616f,   0.97847f },
  {   0.99954f,   0.99170f,   0.98708f },
  {   0.99985f,   0.99723f,   0.99569f },
  {   0.99762f,   0.99762f,   0.99762f },
  {   0.99285f,   0.99285f,   0.99285f },
  {   0.98808f,   0.98808f,   0.98808f },
  {   0.98331f,   0.98331f,   0.98331f },
  {   0.97855f,   0.97855f,   0.97855f },
  {   0.97378f,   0.97378f,   0.97378f },
  {   0.96901f,   0.96901f,   0.96901f },
  {   0.96424f,   0.96424f,   0.96424f },
  {   0.95948f,   0.95948f,   0.95948f },
  {   0.95471f,   0.95471f,   0.95471f },
  {   0.94994f,   0.94994f,   0.94994f },
  {   0.94517f,   0.94517f,   0.94517f },
  {   0.94041f,   0.94041f,   0.94041f },
  {   0.93564f,   0.93564f,   0.93564f },
  {   0.93087f,   0.93087f,   0.93087f },
  {   0.92611f,   0.92611f,   0.92611f },
  {   0.92134f,   0.92134f,   0.92134f },
  {   0.91657f,   0.91657f,   0.91657f },
  {   0.91180f,   0.91180f,   0.91180f },
  {   0.90704f,   0.90704f,   0.90704f },
  {   0.90227f,   0.90227f,   0.90227f },
  {   0.89750f,   0.89750f,   0.89750f },
  {   0.89273f,   0.89273f,   0.89273f },
  {   0.88797f,   0.88797f,   0.88797f },
  {   0.88320f,   0.88320f,   0.88320f },
  {   0.87843f,   0.87843f,   0.87843f },
  {   0.87259f,   0.87259f,   0.87259f },
  {   0.86674f,   0.86674f,   0.86674f },
  {   0.86090f,   0.86090f,   0.86090f },
  {   0.85506f,   0.85506f,   0.85506f },
  {   0.84921f,   0.84921f,   0.84921f },
  {   0.84337f,   0.84337f,   0.84337f },
  {   0.83752f,   0.83752f,   0.83752f },
  {   0.83168f,   0.83168f,   0.83168f },
  {   0.82584f,   0.82584f,   0.82584f },
  {   0.81999f,   0.81999f,   0.81999f },
  {   0.81415f,   0.81415f,   0.81415f },
  {   0.80830f,   0.80830f,   0.80830f },
  {   0.80246f,   0.80246f,   0.80246f },
  {   0.79662f,   0.79662f,   0.79662f },
  {   0.79077f,   0.79077f,   0.79077f },
  {   0.78493f,   0.78493f,   0.78493f },
  {   0.77908f,   0.77908f,   0.77908f },
  {   0.77324f,   0.77324f,   0.77324f },
  {   0.76740f,   0.76740f,   0.76740f },
  {   0.76155f,   0.76155f,   0.76155f },
  {   0.75571f,   0.75571f,   0.75571f },
  {   0.74987f,   0.74987f,   0.74987f },
  {   0.74402f,   0.74402f,   0.74402f },
  {   0.73818f,   0.73818f,   0.73818f },
  {   0.73233f,   0.73233f,   0.73233f },
  {   0.72549f,   0.72549f,   0.72549f },
  {   0.71765f,   0.71765f,   0.71765f },
  {   0.70980f,   0.70980f,   0.70980f },
  {   0.70196f,   0.70196f,   0.70196f },
  {   0.69412f,   0.69412f,   0.69412f },
  {   0.68627f,   0.68627f,   0.68627f },
  {   0.67843f,   0.67843f,   0.67843f },
  {   0.67059f,   0.67059f,   0.67059f },
  {   0.66275f,   0.66275f,   0.66275f },
  {   0.65490f,   0.65490f,   0.65490f },
  {   0.64706f,   0.64706f,   0.64706f },
  {   0.63922f,   0.63922f,   0.63922f },
  {   0.63137f,   0.63137f,   0.63137f },
  {   0.62353f,   0.62353f,   0.62353f },
  {   0.61569f,   0.61569f,   0.61569f },
  {   0.60784f,   0.60784f,   0.60784f },
  {   0.60000f,   0.60000f,   0.60000f },
  {   0.59216f,   0.59216f,   0.59216f },
  {   0.58431f,   0.58431f,   0.58431f },
  {   0.57647f,   0.57647f,   0.57647f },
  {   0.56863f,   0.56863f,   0.56863f },
  {   0.56078f,   0.56078f,   0.56078f },
  {   0.55294f,   0.55294f,   0.55294f },
  {   0.54510f,   0.54510f,   0.54510f },
  {   0.53725f,   0.53725f,   0.53725f },
  {   0.52941f,   0.52941f,   0.52941f },
  {   0.52049f,   0.52049f,   0.52049f },
  {   0.51157f,   0.51157f,   0.51157f },
  {   0.50265f,   0.50265f,   0.50265f },
  {   0.49373f,   0.49373f,   0.49373f },
  {   0.48481f,   0.48481f,   0.48481f },
  {   0.47589f,   0.47589f,   0.47589f },
  {   0.46697f,   0.46697f,   0.46697f },
  {   0.45805f,   0.45805f,   0.45805f },
  {   0.44913f,   0.44913f,   0.44913f },
  {   0.44022f,   0.44022f,   0.44022f },
  {   0.43130f,   0.43130f,   0.43130f },
  {   0.42238f,   0.42238f,   0.42238f },
  {   0.41346f,   0.41346f,   0.41346f },
  {   0.40454f,   0.40454f,   0.40454f },
  {   0.39562f,   0.39562f,   0.39562f },
  {   0.38670f,   0.38670f,   0.38670f },
  {   0.37778f,   0.37778f,   0.37778f },
  {   0.36886f,   0.36886f,   0.36886f },
  {   0.35994f,   0.35994f,   0.35994f },
  {   0.35102f,   0.35102f,   0.35102f },
  {   0.34210f,   0.34210f,   0.34210f },
  {   0.33318f,   0.33318f,   0.33318f },
  {   0.32426f,   0.32426f,   0.32426f },
  {   0.31534f,   0.31534f,   0.31534f },
  {   0.30642f,   0.30642f,   0.30642f },
  {   0.29804f,   0.29804f,   0.29804f },
  {   0.29020f,   0.29020f,   0.29020f },
  {   0.28235f,   0.28235f,   0.28235f },
  {   0.27451f,   0.27451f,   0.27451f },
  {   0.26667f,   0.26667f,   0.26667f },
  {   0.25882f,   0.25882f,   0.25882f },
  {   0.25098f,   0.25098f,   0.25098f },
  {   0.24314f,   0.24314f,   0.24314f },
  {   0.23529f,   0.23529f,   0.23529f },
  {   0.22745f,   0.22745f,   0.22745f },
  {   0.21961f,   0.21961f,   0.21961f },
  {   0.21176f,   0.21176f,   0.21176f },
  {   0.20392f,   0.20392f,   0.20392f },
  {   0.19608f,   0.19608f,   0.19608f },
  {   0.18824f,   0.18824f,   0.18824f },
  {   0.18039f,   0.18039f,   0.18039f },
  {   0.17255f,   0.17255f,   0.17255f },
  {   0.16471f,   0.16471f,   0.16471f },
  {   0.15686f,   0.15686f,   0.15686f },
  {   0.14902f,   0.14902f,   0.14902f },
  {   0.14118f,   0.14118f,   0.14118f },
  {   0.13333f,   0.13333f,   0.13333f },
  {   0.12549f,   0.12549f,   0.12549f },
  {   0.11765f,   0.11765f,   0.11765f },
  {   0.10980f,   0.10980f,   0.10980f },
  {   0.10196f,   0.10196f,   0.10196f },
};
// clang-format on

} // namespace goom::utils::colordata
