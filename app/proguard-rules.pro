# Add project specific ProGuard rules here.

# Keep JNI native methods
-keepclasseswithmembernames class * {
    native <methods>;
}

# Keep NativeDumper and its callback interface
-keep class com.il2cpp.dumper.NativeDumper { *; }
-keep class com.il2cpp.dumper.NativeDumper$LogCallback { *; }

# Keep Room entities and DAOs
-keep class com.il2cpp.dumper.data.db.** { *; }

# Keep kotlinx.serialization
-keepattributes *Annotation*, InnerClasses
-dontnote kotlinx.serialization.AnnotationsKt
-keepclassmembers class kotlinx.serialization.json.** { *** Companion; }
-keepclasseswithmembers class kotlinx.serialization.json.** {
    kotlinx.serialization.KSerializer serializer(...);
}
-keepclassmembers class * {
    @kotlinx.serialization.Serializable <fields>;
}

# Keep BuildConfig
-keep class com.il2cpp.dumper.BuildConfig { *; }

# Preserve line number info for crash reports
-keepattributes SourceFile,LineNumberTable
-renamesourcefileattribute SourceFile
