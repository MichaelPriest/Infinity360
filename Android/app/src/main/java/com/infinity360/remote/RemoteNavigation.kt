package com.infinity360.remote

import android.content.Context
import androidx.appcompat.app.AppCompatDelegate
import androidx.core.os.LocaleListCompat
import com.google.android.material.bottomnavigation.BottomNavigationView
import com.google.android.material.navigation.NavigationBarView

/** SDK/Jetpack navigation and locale bridge shared with the existing Java controller. */
object RemoteNavigation {
    private val ids = intArrayOf(R.id.nav_control, R.id.nav_catalog, R.id.nav_system, -1, R.id.nav_store, R.id.nav_home)
    @JvmStatic fun create(context: Context): BottomNavigationView = BottomNavigationView(context).apply {
        inflateMenu(R.menu.remote_navigation)
        labelVisibilityMode = NavigationBarView.LABEL_VISIBILITY_LABELED
    }
    @JvmStatic fun index(id: Int): Int = ids.indexOf(id)
    @JvmStatic fun select(view: BottomNavigationView?, index: Int) {
        if (view != null && index in ids.indices && ids[index] != -1 && view.selectedItemId != ids[index]) view.selectedItemId = ids[index]
    }
    @JvmStatic fun applyTheme(context: Context) {
        val mode=context.getSharedPreferences("appearance",Context.MODE_PRIVATE).getInt("mode",AppCompatDelegate.MODE_NIGHT_YES)
        AppCompatDelegate.setDefaultNightMode(mode)
    }
    @JvmStatic fun theme(context: Context,choice: Int) {
        val mode=listOf(AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM,AppCompatDelegate.MODE_NIGHT_YES,AppCompatDelegate.MODE_NIGHT_NO).getOrNull(choice) ?: return
        context.getSharedPreferences("appearance",Context.MODE_PRIVATE).edit().putInt("mode",mode).apply()
        AppCompatDelegate.setDefaultNightMode(mode)
    }
    @JvmStatic fun locale(choice: Int) {
        val language = listOf("", "pt-BR", "en", "es").getOrNull(choice) ?: return
        AppCompatDelegate.setApplicationLocales(LocaleListCompat.forLanguageTags(language))
    }
}
