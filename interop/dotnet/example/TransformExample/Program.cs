// See https://aka.ms/new-console-template for more information

using System.Xml.Serialization;
using System;

// Note the namespace defined as part of the SWIG-generated files
// that are referenced through the FiftyoneDegrees.Common.csproj wrapper project

using FiftyoneDegrees.Common;

Console.WriteLine("Current Dir: " + Directory.GetCurrentDirectory());

// MapStringStringSwig type is an ordinary key-value pair container
// that is corresponds to the C++ std::map<std::string, std::string> type
// Can be used as Dictionary<string,string> as demonstrated below
void outputResult(MapStringStringSwig result) {
    foreach (var k in result.Keys)
    {
        Console.WriteLine(k + ": " + result[k]);
    }
    Console.WriteLine("");
}

// Instantiate the Transform object with a default buffer size of 1024
// Optionaly set a working buffer size like `var transform = new Transform(2048);`
// It will be automatically increased if needed 
// The instance is non-threadsafe, but reusable within a single thread
// i.e. can be used multiple times as in the example below
var transform = new Transform();
{
    Console.WriteLine("From GHEV:");
    var result = transform.fromJsonGHEV("{\"architecture\":\"x86\",\"brands\":[{\"brand\":\"Chromium\",\"version\":\"128\"},{\"brand\":\"Not;A=Brand\",\"version\":\"24\"},{\"brand\":\"Google Chrome\",\"version\":\"128\"}],\"fullVersionList\":[{\"brand\":\"Chromium\",\"version\":\"128.0.6613.84\"},{\"brand\":\"Not;A=Brand\",\"version\":\"24.0.0.0\"},{\"brand\":\"Google Chrome\",\"version\":\"128.0.6613.84\"}],\"mobile\":false,\"model\":\"\",\"platform\":\"macOS\",\"platformVersion\":\"14.6.1\"}");
    outputResult(result);
}
{
    Console.WriteLine("From SUA:");
    var result = transform.fromSUA("{\"browsers\":[{\"brand\":\"Chromium\",\"version\":[\"124\",\"0\",\"6367\",\"82\"]},{\"brand\":\"Google Chrome\",\"version\":[\"124\",\"0\",\"6367\",\"82\"]},{\"brand\":\"Not-A.Brand\",\"version\":[\"99\",\"0\",\"0\",\"0\"]}],\"platform\":{\"brand\":\"Android\",\"version\":[\"14\",\"0\",\"0\"]},\"mobile\":1,\"model\":\"SM-G998U\",\"source\":2}");
    outputResult(result);
}
{
    Console.WriteLine("From base64 GHEV:");
    var result = transform.fromBase64GHEV("eyJhcmNoaXRlY3R1cmUiOiJ4ODYiLCJiaXRuZXNzIjoiNjQiLCJicmFuZHMiOlt7ImJyYW5kIjoiQ2hyb21pdW0iLCJ2ZXJzaW9uIjoiMTI4In0seyJicmFuZCI6Ik5vdDtBPUJyYW5kIiwidmVyc2lvbiI6IjI0In0seyJicmFuZCI6Ikdvb2dsZSBDaHJvbWUiLCJ2ZXJzaW9uIjoiMTI4In1dLCJmdWxsVmVyc2lvbkxpc3QiOlt7ImJyYW5kIjoiQ2hyb21pdW0iLCJ2ZXJzaW9uIjoiMTI4LjAuNjYxMy44NCJ9LHsiYnJhbmQiOiJOb3Q7QT1CcmFuZCIsInZlcnNpb24iOiIyNC4wLjAuMCJ9LHsiYnJhbmQiOiJHb29nbGUgQ2hyb21lIiwidmVyc2lvbiI6IjEyOC4wLjY2MTMuODQifV0sIm1vYmlsZSI6ZmFsc2UsIm1vZGVsIjoiTWFjQm9vayBQcm8iLCJwbGF0Zm9ybSI6Im1hY09TIn0=");
    outputResult(result);
}

