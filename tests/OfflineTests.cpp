// SPDX-License-Identifier: AGPL-3.0-or-later
#include "dsp/PeakBandStft.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using spectral_decay::PeakBandStft;
struct Result { std::vector<std::vector<float>> out; double cpuUs=0.0; std::size_t callbacks=0; int latency=0; };
Result render(double fs,std::size_t block,float decay,float center=1000.0f,float q=1.0f,float mu=0.5f,bool foreign=true,double sourceHz=1000.0)
{
    const std::size_t signalLength=static_cast<std::size_t>(fs*0.55), total=signalLength+static_cast<std::size_t>(fs*2.35)+PeakBandStft::latency;
    PeakBandStft dsp;dsp.prepare(fs,1);dsp.setBandFrequencyHz(center);dsp.setBandQ(q);dsp.setBandDecay(decay);dsp.setToneNoise(mu);
    Result r;r.latency=dsp.latencySamples();r.out.assign(1,std::vector<float>(total));std::vector<float> b(block);float* ptr[1]{b.data()};
    for(std::size_t pos=0;pos<total;){auto n=std::min(block,total-pos);for(std::size_t i=0;i<n;++i){auto t=pos+i;double v=0;if(t<signalLength){double time=double(t)/fs;v=0.24*std::exp(-time/0.11)*std::sin(2*3.141592653589793*sourceHz*time);if(foreign)v+=0.08*std::sin(2*3.141592653589793*8000*time);}b[i]=float(v);r.out[0][t]=b[i];}auto start=std::chrono::steady_clock::now();dsp.process(ptr,n);r.cpuUs+=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();++r.callbacks;std::copy_n(b.data(),n,r.out[0].data()+pos);pos+=n;}
    return r;
}
Result renderPercussive(double fs, std::size_t block, float decay)
{
    const std::size_t signalLength = static_cast<std::size_t>(fs * 0.08);
    const std::size_t total = signalLength + static_cast<std::size_t>(fs * 2.35) + PeakBandStft::latency;
    PeakBandStft dsp;
    dsp.prepare(fs, 1);
    dsp.setBandFrequencyHz(1000.0f);
    dsp.setBandQ(1.0f);
    dsp.setBandDecay(decay);
    dsp.setToneNoise(0.5f);
    Result r;
    r.latency = dsp.latencySamples();
    r.out.assign(1, std::vector<float>(total));
    std::vector<float> buffer(block);
    float* channel[1]{buffer.data()};
    for (std::size_t pos = 0; pos < total;)
    {
        const auto count = std::min(block, total - pos);
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto sample = pos + i;
            const double time = static_cast<double>(sample) / fs;
            float input = 0.0f;
            if (sample < signalLength)
            {
                const double body = 0.24 * std::exp(-time / 0.035) *
                    std::sin(2.0 * 3.141592653589793 * 1000.0 * time);
                input = static_cast<float>(body + (sample < 3 ? (sample == 1 ? 0.9 : 0.0) : 0.0));
            }
            buffer[i] = input;
            r.out[0][sample] = input;
        }
        const auto start = std::chrono::steady_clock::now();
        dsp.process(channel, count);
        r.cpuUs += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
        ++r.callbacks;
        std::copy_n(buffer.data(), count, r.out[0].data() + pos);
        pos += count;
    }
    return r;
}
Result renderFullLoad(double fs, std::size_t block, float amplitude, float decay)
{
    const std::size_t signalLength = static_cast<std::size_t>(fs * 0.55);
    const std::size_t total = signalLength + static_cast<std::size_t>(fs * 2.35) + PeakBandStft::latency;
    PeakBandStft dsp;
    dsp.prepare(fs, 1);
    dsp.setBandFrequencyHz(1000.0f);
    dsp.setBandQ(1.0f);
    dsp.setBandDecay(decay);
    dsp.setToneNoise(0.0f);
    Result r;
    r.latency = dsp.latencySamples();
    r.out.assign(1, std::vector<float>(total));
    std::vector<float> buffer(block);
    float* channel[1]{buffer.data()};
    for (std::size_t pos = 0; pos < total;)
    {
        const auto count = std::min(block, total - pos);
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto sample = pos + i;
            const float input = sample < signalLength
                ? amplitude * std::sin(static_cast<float>(2.0 * 3.141592653589793 * 1000.0 * sample / fs))
                : 0.0f;
            buffer[i] = input;
            r.out[0][sample] = input;
        }
        const auto start = std::chrono::steady_clock::now();
        dsp.process(channel, count);
        r.cpuUs += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
        ++r.callbacks;
        std::copy_n(buffer.data(), count, r.out[0].data() + pos);
        pos += count;
    }
    return r;
}
double absoluteEnergy(const Result& r, double fs, double begin, double end)
{
    const auto first = std::min(r.out[0].size(), static_cast<std::size_t>(begin * fs) + r.latency);
    const auto last = std::min(r.out[0].size(), static_cast<std::size_t>(end * fs) + r.latency);
    long double sum = 0.0;
    for (auto i = first; i < last; ++i)
        sum += static_cast<double>(r.out[0][i]) * r.out[0][i];
    return last > first ? static_cast<double>(sum / (last - first)) : 0.0;
}
double maximumAdjacentDifference(const std::vector<float>& samples, std::size_t begin, std::size_t end)
{
    double maximum = 0.0;
    end = std::min(end, samples.size());
    for (auto i = std::max<std::size_t>(1, begin); i < end; ++i)
        maximum = std::max(maximum, std::abs(static_cast<double>(samples[i]) - samples[i - 1]));
    return maximum;
}
double energy(const Result&r,double fs,double begin,double end,double freqLo=0,double freqHi=0)
{
    const auto a=std::min(r.out[0].size(),static_cast<std::size_t>((0.55+begin)*fs)+r.latency),b=std::min(r.out[0].size(),static_cast<std::size_t>((0.55+end)*fs)+r.latency);long double e=0;std::size_t count=0;
    if(freqLo==0){for(auto i=a;i<b;++i){double x=r.out[0][i];e+=x*x;++count;}}
    else{auto goertzel=[&](double hz){double s1=0,c=2*std::cos(2*3.141592653589793*hz/fs),s2=0;for(auto i=a;i<b;++i){double v=r.out[0][i]+c*s1-s2;s2=s1;s1=v;}return std::max(0.0,s1*s1+s2*s2-c*s1*s2);};const double scale=b>a?4.0/(double(b-a)*double(b-a)):0.0;e=goertzel(freqLo)*scale;count=1;if(freqHi>0){e+=goertzel(freqHi)*scale;count=2;}}
    return count?double(e/count):0;
}

struct StereoResult { std::vector<std::vector<float>> out; double cpuUs=0.0; std::size_t callbacks=0; };
StereoResult renderStereo(double fs,std::size_t block,int mode)
{
    const std::size_t source=static_cast<std::size_t>(fs*0.55),total=source+static_cast<std::size_t>(fs*2.35)+PeakBandStft::latency;
    PeakBandStft dsp;dsp.prepare(fs,2);dsp.setBandFrequencyHz(1000);dsp.setBandQ(1);dsp.setBandDecay(3);dsp.setToneNoise(.5f);
    StereoResult r;r.out.assign(2,std::vector<float>(total));std::vector<float> l(block),rr(block);float* pointers[2]{l.data(),rr.data()};
    for(std::size_t pos=0;pos<total;){auto n=std::min(block,total-pos);for(std::size_t i=0;i<n;++i){const auto t=pos+i;float x=t<source?float(.2*std::exp(-double(t)/fs/.11)*std::sin(2*3.141592653589793*1000*t/fs)):0.0f;l[i]=x;rr[i]=mode==0?x:(mode==1?-x:.5f*x);r.out[0][t]=x;r.out[1][t]=rr[i];}auto begin=std::chrono::steady_clock::now();dsp.process(pointers,n);r.cpuUs+=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count();++r.callbacks;std::copy_n(l.data(),n,r.out[0].data()+pos);std::copy_n(rr.data(),n,r.out[1].data()+pos);pos+=n;}
    return r;
}
bool allFinite(const std::vector<float>&v){return std::all_of(v.begin(),v.end(),[](float x){return std::isfinite(x);});}
double windowDifference(const Result&a,const Result&b,double fs,double begin,double end){auto first=std::min(a.out[0].size(),static_cast<std::size_t>(begin*fs)+a.latency),last=std::min(a.out[0].size(),static_cast<std::size_t>(end*fs)+a.latency);long double e=0;for(auto i=first;i<last;++i){double d=a.out[0][i]-b.out[0][i];e+=d*d;}return last>first?std::sqrt(double(e/(last-first))):0;}
double samplePeak(const Result&r){double p=0;for(float x:r.out[0])p=std::max(p,std::abs(double(x)));return p;}
double timeEnergy(const Result&r,double fs,double begin,double end){auto a=std::min(r.out[0].size(),static_cast<std::size_t>(begin*fs)+r.latency),b=std::min(r.out[0].size(),static_cast<std::size_t>(end*fs)+r.latency);long double e=0;for(auto i=a;i<b;++i)e+=double(r.out[0][i])*r.out[0][i];return b>a?double(e/(b-a)):0;}
bool finite(const Result&r){return std::all_of(r.out[0].begin(),r.out[0].end(),[](float x){return std::isfinite(x);});}
}
int main(int argc,char**argv)
{
    const std::string path=argc==3&&std::string(argv[1])=="--report"?argv[2]:"offline-metrics.csv";std::ofstream csv(path);if(!csv){std::cerr<<"Cannot write "<<path<<'\n';return 2;}csv<<"fs,block,decay,metric,value,status\n";
    double maxNull=0;std::size_t lines=1;bool valid=true;
    for(double fs:{44100.0,48000.0})for(std::size_t block:{1u,64u,257u,512u}){
        auto neutral=render(fs,block,0,4000,8,1);double peak=0;for(std::size_t i=0;i<neutral.out[0].size();++i){double ref=i>=neutral.latency&&i-neutral.latency<neutral.out[0].size()?((i-neutral.latency<static_cast<std::size_t>(fs*.55))?0.24*std::exp(-double(i-neutral.latency)/fs/.11)*std::sin(2*3.141592653589793*1000*(i-neutral.latency)/fs)+0.08*std::sin(2*3.141592653589793*8000*(i-neutral.latency)/fs):0):0;peak=std::max(peak,std::abs(double(neutral.out[0][i])-ref));}maxNull=std::max(maxNull,peak);csv<<fs<<','<<block<<",0,neutral_null_peak,"<<peak<<",MEASUREMENT_ONLY\n";++lines;
        auto neutralTarget=render(fs,block,0.0f,1000.0f,1.0f,0.5f,false);auto minus2=render(fs,block,-2.5f,1000.0f,1.0f,0.5f,false),minus5=render(fs,block,-5.0f,1000.0f,1.0f,0.5f,false),plus2=render(fs,block,2.5f),plus5=render(fs,block,5.0f);
        double e0=timeEnergy(neutralTarget,fs,.2,.5),e0Positive=energy(neutralTarget,fs,.25,.5,1000),em2=timeEnergy(minus2,fs,.2,.5),em5=timeEnergy(minus5,fs,.2,.5),ep2=energy(plus2,fs,.25,.5,1000),ep5=energy(plus5,fs,.25,.5,1000);
        csv<<fs<<','<<block<<",0,shortening_reference_energy,"<<e0<<",MEASUREMENT_ONLY\n"<<fs<<','<<block<<",0,extension_reference_1k_power,"<<e0Positive<<",MEASUREMENT_ONLY\n"<<fs<<','<<block<<",-2.5,late_energy,"<<em2<<",MEASUREMENT_ONLY\n"<<fs<<','<<block<<",-5,late_energy,"<<em5<<",MEASUREMENT_ONLY\n"<<fs<<','<<block<<",2.5,late_energy,"<<ep2<<",MEASUREMENT_ONLY\n"<<fs<<','<<block<<",5,late_energy,"<<ep5<<",MEASUREMENT_ONLY\n";lines+=6;
        const bool ok=finite(minus2)&&finite(minus5)&&finite(plus2)&&finite(plus5)&&neutral.latency==2048&&em5<em2&&em2<e0&&ep5>ep2&&ep2>e0Positive;valid=valid&&ok;
        const double cpu=plus5.cpuUs/std::max<std::size_t>(1,plus5.callbacks);csv<<fs<<','<<block<<",5,callback_mean_us,"<<cpu<<",MEASUREMENT_ONLY\n";++lines;
        std::array<Result, 5> morphs;
        constexpr std::array<float, 5> morphValues{0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        for (std::size_t i = 0; i < morphValues.size(); ++i)
        {
            morphs[i] = render(fs, block, 3.0f, 1000, 1, morphValues[i]);
            valid = valid && finite(morphs[i]);
            csv << fs << ',' << block << ",3,tone_noise_" << morphValues[i] << ','
                << energy(morphs[i], fs, .1, .5) << ",MEASUREMENT_ONLY\n";
            ++lines;
        }
        double previousToneDistance = -1.0, previousNoiseDistance = std::numeric_limits<double>::infinity();
        for (std::size_t i = 0; i < morphs.size(); ++i)
        {
            const double toneDistance = windowDifference(morphs[i], morphs.front(), fs, .1, .5);
            const double noiseDistance = windowDifference(morphs[i], morphs.back(), fs, .1, .5);
            if (i > 0)
                valid = valid && toneDistance > previousToneDistance && noiseDistance < previousNoiseDistance;
            csv << fs << ',' << block << ",3,tone_l2_distance_" << morphValues[i] << ','
                << toneDistance << ",MEASUREMENT_ONLY\n";
            csv << fs << ',' << block << ",3,noise_l2_distance_" << morphValues[i] << ','
                << noiseDistance << ",MEASUREMENT_ONLY\n";
            lines += 2;
            previousToneDistance = toneDistance;
            previousNoiseDistance = noiseDistance;
        }
    }
    for(double fs:{44100.0,48000.0})
    {
        for(float decay:{-4.0f,4.0f})
        {
            auto selected=render(fs,257,decay,2500.0f,1.0f,0.5f,false,2500.0);
            auto displaced=render(fs,257,decay,400.0f,1.0f,0.5f,false,2500.0);
            const double selectedMetric=decay>0?energy(selected,fs,.25,.5,2500):timeEnergy(selected,fs,.2,.5);
            const double displacedMetric=decay>0?energy(displaced,fs,.25,.5,2500):timeEnergy(displaced,fs,.2,.5);
            const bool pass=decay>0?selectedMetric>displacedMetric:selectedMetric<displacedMetric;
            valid=valid&&pass;csv<<fs<<",257,"<<decay<<",center_selectivity,"<<selectedMetric<<",MEASUREMENT_ONLY\n"<<fs<<",257,"<<decay<<",displaced_center,"<<displacedMetric<<",MEASUREMENT_ONLY\n";lines+=2;
        }
        const auto neutralPercussive = renderPercussive(fs, 257, 0.0f);
        const auto shortPercussive = renderPercussive(fs, 257, -5.0f);
        const auto mediumPercussive = renderPercussive(fs, 257, -2.5f);
        const double onsetNeutral = absoluteEnergy(neutralPercussive, fs, 0.0, 0.02);
        const double onsetRatio = absoluteEnergy(shortPercussive, fs, 0.0, 0.02) / std::max(1.0e-20, onsetNeutral);
        const double staticHalfLevelRatio = 0.25;
        const double tailNeutral = absoluteEnergy(neutralPercussive, fs, 0.04, 0.08);
        const double tailMediumRatio = absoluteEnergy(mediumPercussive, fs, 0.04, 0.08) / std::max(1.0e-20, tailNeutral);
        const double tailShortRatio = absoluteEnergy(shortPercussive, fs, 0.04, 0.08) / std::max(1.0e-20, tailNeutral);
        valid = valid && onsetRatio > 0.70 && onsetRatio > staticHalfLevelRatio &&
                tailMediumRatio < 1.0 && tailShortRatio < tailMediumRatio;
        csv << fs << ",257,-5,percussive_onset_to_neutral_energy_ratio," << onsetRatio << ",MEASUREMENT_ONLY\n"
            << fs << ",257,-5,percussive_onset_to_static_half_level_ratio," << onsetRatio / staticHalfLevelRatio << ",MEASUREMENT_ONLY\n"
            << fs << ",257,-2.5,percussive_late_body_to_neutral_energy_ratio," << tailMediumRatio << ",MEASUREMENT_ONLY\n"
            << fs << ",257,-5,percussive_late_body_to_neutral_energy_ratio," << tailShortRatio << ",MEASUREMENT_ONLY\n";
        lines += 4;
        auto broadPlus=render(fs,257,4.0f,1000.0f,0.4f,0.5f,false,2000.0),narrowPlus=render(fs,257,4.0f,1000.0f,8.0f,0.5f,false,2000.0);
        auto broadMinus=render(fs,257,-4.0f,1000.0f,0.4f,0.5f,false,2000.0),narrowMinus=render(fs,257,-4.0f,1000.0f,8.0f,0.5f,false,2000.0);
        const double bp=energy(broadPlus,fs,.25,.5,2000),np=energy(narrowPlus,fs,.25,.5,2000),bm=timeEnergy(broadMinus,fs,.2,.5),nm=timeEnergy(narrowMinus,fs,.2,.5);
        valid=valid&&bp>np&&nm>bm;csv<<fs<<",257,4,q_broad_offcenter,"<<bp<<",MEASUREMENT_ONLY\n"<<fs<<",257,4,q_narrow_offcenter,"<<np<<",MEASUREMENT_ONLY\n"<<fs<<",257,-4,q_broad_offcenter,"<<bm<<",MEASUREMENT_ONLY\n"<<fs<<",257,-4,q_narrow_offcenter,"<<nm<<",MEASUREMENT_ONLY\n";lines+=4;
        const auto bounded = render(fs, 257, 5.0f);
        for (const float amplitude : {0.85f, 0.95f})
        {
            const auto fullLoad = renderFullLoad(fs, 257, amplitude, 5.0f);
            const auto dryReference = renderFullLoad(fs, 257, amplitude, 0.0f);
            bool withinCeiling = true;
            double outputPeak = 0.0;
            std::size_t protectionInterventions = 0;
            for (std::size_t i = 0; i < fullLoad.out[0].size(); ++i)
            {
                const double dry = std::abs(dryReference.out[0][i]);
                const double actual = std::abs(fullLoad.out[0][i]);
                const double allowed = std::max<double>(PeakBandStft::tailOutputCeiling, dry);
                outputPeak = std::max(outputPeak, actual);
                withinCeiling = withinCeiling && actual <= allowed + 1.0e-5;
                if (dry < PeakBandStft::tailOutputCeiling - 1.0e-3 &&
                    actual >= PeakBandStft::tailOutputCeiling - 1.0e-4 &&
                    std::abs(fullLoad.out[0][i] - dryReference.out[0][i]) > 1.0e-3)
                    ++protectionInterventions;
            }
            valid = valid && withinCeiling;
            csv << fs << ",257,5,full_load_" << amplitude << "_output_peak," << outputPeak << ",MEASUREMENT_ONLY\n"
                << fs << ",257,5,full_load_" << amplitude << "_within_sample_ceiling," << (withinCeiling ? 1 : 0) << ",MEASUREMENT_ONLY\n"
                << fs << ",257,5,full_load_" << amplitude << "_protection_interventions," << protectionInterventions << ",MEASUREMENT_ONLY\n";
            lines += 3;
            if (amplitude == 0.85f)
                valid = valid && protectionInterventions > 0;
        }
        const auto dryReference = render(fs, 257, 0.0f);
        const auto signalEnd = static_cast<std::size_t>(std::llround(0.55 * fs)) - 1;
        const auto cutoff = signalEnd + static_cast<std::size_t>(std::llround(PeakBandStft::maximumTailSeconds * fs)) +
                            static_cast<std::size_t>(bounded.latency);
        bool endedAtDeadline = cutoff < bounded.out[0].size();
        std::vector<float> tailDifference(bounded.out[0].size());
        for (std::size_t i = 0; i < bounded.out[0].size(); ++i)
            tailDifference[i] = bounded.out[0][i] - dryReference.out[0][i];
        for (std::size_t i = cutoff; i < bounded.out[0].size(); ++i)
            endedAtDeadline = endedAtDeadline && tailDifference[i] == 0.0f;
        const auto fadeBegin = cutoff > PeakBandStft::hopSize ? cutoff - PeakBandStft::hopSize : 0;
        const double deadlineFadeStep = maximumAdjacentDifference(tailDifference, fadeBegin, cutoff + 1);
        csv << fs << ",257,5,tail_zero_at_exact_2s_deadline," << (endedAtDeadline ? 1 : 0) << ",MEASUREMENT_ONLY\n"
            << fs << ",257,5,deadline_fade_max_sample_step," << deadlineFadeStep << ",MEASUREMENT_ONLY\n";
        lines += 2;
        valid = valid && endedAtDeadline;
        auto negativeSilence=render(fs,257,-5.0f,1000,1,.5f,false);const auto dryEnd=static_cast<std::size_t>((0.55+PeakBandStft::fftSize/fs)*fs)+negativeSilence.latency;const bool noNegativeTail=std::all_of(negativeSilence.out[0].begin()+static_cast<std::ptrdiff_t>(std::min(dryEnd,negativeSilence.out[0].size())),negativeSilence.out[0].end(),[](float x){return x==0.0f;});csv<<fs<<",257,-5,no_tail_after_input_silence,"<<(noNegativeTail?1:0)<<",MEASUREMENT_ONLY\n";++lines;valid=valid&&noNegativeTail;
        for(float mu:{0.0f,0.25f,0.5f,0.75f}){auto a=render(fs,257,4.0f,1000,1,mu),b=render(fs,257,4.0f,1000,1,mu+0.25f);const double diff=windowDifference(a,b,fs,.05,.5);csv<<fs<<",257,4,tone_noise_adjacent_diff_"<<mu<<','<<diff<<",MEASUREMENT_ONLY\n";++lines;valid=valid&&finite(a)&&finite(b)&&diff>0.0;}
    }
    for(double fs:{44100.0,48000.0})for(std::size_t block:{1u,64u,257u,512u})
    {
        auto identical=renderStereo(fs,block,0),opposite=renderStereo(fs,block,1),panned=renderStereo(fs,block,2);bool same=true,anti=true;for(std::size_t i=0;i<identical.out[0].size();++i){same=same&&identical.out[0][i]==identical.out[1][i];anti=anti&&opposite.out[0][i]==-opposite.out[1][i];}valid=valid&&same&&anti&&allFinite(panned.out[0])&&allFinite(panned.out[1]);const double cpu=identical.cpuUs/std::max<std::size_t>(1,identical.callbacks);csv<<fs<<','<<block<<",3,stereo_identical,"<<(same?1:0)<<",MEASUREMENT_ONLY\n"<<fs<<','<<block<<",3,stereo_antiphase,"<<(anti?1:0)<<",MEASUREMENT_ONLY\n"<<fs<<','<<block<<",3,stereo_callback_mean_us,"<<cpu<<",MEASUREMENT_ONLY\n";lines+=3;
    }
    for(double fs:{44100.0,48000.0})for(float decay:{-3.0f,3.0f})for(float mu:{0.0f,.5f,1.0f}){auto reference=render(fs,1,decay,1000,1,mu);for(std::size_t block:{64u,257u,512u}){auto candidate=render(fs,block,decay,1000,1,mu);const bool same=reference.out[0]==candidate.out[0];valid=valid&&same;csv<<fs<<','<<block<<','<<decay<<",block_invariant_"<<mu<<','<<(same?0:windowDifference(reference,candidate,fs,0,2.0))<<",MEASUREMENT_ONLY\n";++lines;}}
    {
        constexpr double fs = 48000.0;
        PeakBandStft dsp;
        dsp.prepare(fs, 1);
        dsp.setBandFrequencyHz(1000.0f);
        dsp.setBandQ(1.0f);
        dsp.setBandDecay(3.0f);
        std::vector<float> output(48000);
        float sample = 0.0f;
        float* channel[1]{&sample};
        for (std::size_t i = 0; i < output.size(); ++i)
        {
            sample = i < 24000 ? static_cast<float>(0.2 * std::sin(2.0 * 3.141592653589793 * 1000.0 * i / fs)) : 0.0f;
            const float target = i < 24000 ? 0.0f : std::min(1.0f, static_cast<float>(i - 24000) / 12000.0f);
            dsp.setToneNoise(target);
            dsp.process(channel, 1);
            output[i] = sample;
        }
        const double automationStep = maximumAdjacentDifference(output, 24000 + PeakBandStft::latency, output.size());
        const bool continuousToneNoise = allFinite(output) && automationStep < 0.1;
        valid = valid && continuousToneNoise;
        csv << fs << ",1,3,tone_noise_automation_max_sample_step," << automationStep << ",MEASUREMENT_ONLY\n";
        ++lines;
    }
    {
        constexpr double fs = 48000.0;
        constexpr std::size_t sourceLength = 14400;
        constexpr std::size_t automationAt = 16800;
        constexpr std::size_t total = 62400;
        PeakBandStft dsp;
        dsp.prepare(fs, 1);
        dsp.setBandFrequencyHz(1000.0f);
        dsp.setBandQ(1.0f);
        dsp.setBandDecay(5.0f);
        dsp.setToneNoise(0.5f);
        std::vector<float> buffer(257), output(total);
        float* channel[1]{buffer.data()};
        for (std::size_t pos = 0; pos < total;)
        {
            const auto count = std::min(buffer.size(), total - pos);
            if (pos >= automationAt)
                dsp.setBandDecay(0.0f);
            for (std::size_t i = 0; i < count; ++i)
            {
                const auto t = pos + i;
                buffer[i] = t < sourceLength
                    ? static_cast<float>(0.24 * std::exp(-static_cast<double>(t) / fs / 0.11) *
                        std::sin(2.0 * 3.141592653589793 * 1000.0 * t / fs))
                    : 0.0f;
            }
            dsp.process(channel, count);
            std::copy_n(buffer.data(), count, output.data() + pos);
            pos += count;
        }
        const double decayAutomationStep = maximumAdjacentDifference(output, automationAt + PeakBandStft::latency, output.size());
        const auto neutralBegin = static_cast<std::size_t>(0.9 * fs) + PeakBandStft::latency;
        const bool returnedToNeutral = std::all_of(output.begin() + static_cast<std::ptrdiff_t>(neutralBegin), output.end(),
                                                    [](float value) { return value == 0.0f; });
        const bool smoothReturn = allFinite(output) && decayAutomationStep < 0.05 && returnedToNeutral;
        valid = valid && smoothReturn;
        csv << fs << ",257,0,decay_to_neutral_max_sample_step," << decayAutomationStep << ",MEASUREMENT_ONLY\n"
            << fs << ",257,0,decay_to_neutral_exact_silence," << (returnedToNeutral ? 1 : 0) << ",MEASUREMENT_ONLY\n";
        lines += 2;
    }
    for(float q:{0.4f,1.0f,8.0f}){const auto w=PeakBandStft::computeBandWeight(2000,1000,q);csv<<48000<<",0,0,weight_q_"<<q<<','<<w<<",MEASUREMENT_ONLY\n";++lines;}
    csv.flush();csv.close();std::ifstream countFile(path);std::size_t csvLineCount=0;std::string ignored;while(std::getline(countFile,ignored))++csvLineCount;std::cout<<std::setprecision(10)<<"Metrics rows="<<(csvLineCount?csvLineCount-1:0)<<", neutral max null peak="<<maxNull<<" ("<<(maxNull>0?20*std::log10(maxNull):-INFINITY)<<" dBFS), latency=2048; AK2/AK3 directional comparisons="<<(valid?"passed":"failed")<<"; CSV="<<path<<'\n';return valid?0:1;
}
